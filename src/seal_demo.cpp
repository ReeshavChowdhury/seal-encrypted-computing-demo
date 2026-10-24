#include <seal/seal.h>
#include <emscripten/bind.h>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace seal;
using namespace emscripten;

val compute(int a, int b, bool multiply)
{
    std::string stage = "parameter setup";
    try
    {
        if (a < 0 || a > 999 || b < 0 || b > 999)
        {
            throw std::invalid_argument("Inputs must be integers from 0 to 999.");
        }

        EncryptionParameters parms(scheme_type::bfv);
        parms.set_poly_modulus_degree(4096);
        parms.set_coeff_modulus(CoeffModulus::BFVDefault(4096));
        parms.set_plain_modulus(PlainModulus::Batching(4096, 20));

        stage = "context creation";
        SEALContext context(parms);
        stage = "key generation";
        KeyGenerator keygen(context);
        SecretKey secret_key = keygen.secret_key();
        PublicKey public_key;
        stage = "public key generation";
        keygen.create_public_key(public_key);
        RelinKeys relin_keys;
        stage = "relinearization key generation";
        keygen.create_relin_keys(relin_keys);

        stage = "encryptor setup";
        Encryptor encryptor(context, public_key);
        Evaluator evaluator(context);
        Decryptor decryptor(context, secret_key);

        std::stringstream input_a;
        std::stringstream input_b;
        input_a << std::hex << a;
        input_b << std::hex << b;
        Ciphertext encrypted_a;
        Ciphertext encrypted_b;
        stage = "input encryption";
        encryptor.encrypt(Plaintext(input_a.str()), encrypted_a);
        encryptor.encrypt(Plaintext(input_b.str()), encrypted_b);

        if (multiply)
        {
            stage = "encrypted multiplication";
            evaluator.multiply_inplace(encrypted_a, encrypted_b);
            evaluator.relinearize_inplace(encrypted_a, relin_keys);
        }
        else
        {
            stage = "encrypted addition";
            evaluator.add_inplace(encrypted_a, encrypted_b);
        }

        Plaintext decrypted;
        stage = "result decryption";
        decryptor.decrypt(encrypted_a, decrypted);
        const auto result = std::stoll(decrypted.to_string(), nullptr, 16);
        std::stringstream serialized;
        encrypted_a.save(serialized);

        val output = val::object();
        output.set("value", static_cast<double>(result));
        output.set("ciphertextBytes", static_cast<double>(serialized.str().size()));
        return output;
    }
    catch (const std::exception &error)
    {
        val output = val::object();
        output.set("error", stage + ": " + error.what());
        return output;
    }
}

val compute_ckks(val input_a, val input_b, int operation, int steps)
{
    std::string stage = "parameter setup";
    try
    {
        if (operation < 0 || operation > 2)
        {
            throw std::invalid_argument("Choose CKKS addition, multiplication, or rotation.");
        }
        if (operation == 2 && (steps < -3 || steps > 3))
        {
            throw std::invalid_argument("Rotation steps must be between -3 and 3.");
        }

        auto read_vector = [](val values, const char *label) {
            const auto length = values["length"].as<unsigned>();
            if (length != 4)
            {
                throw std::invalid_argument(std::string(label) + " must contain four numbers.");
            }
            std::vector<double> result;
            result.reserve(length);
            for (unsigned i = 0; i < length; ++i)
            {
                const auto value = values[i].as<double>();
                if (!std::isfinite(value) || std::abs(value) > 100.0)
                {
                    throw std::invalid_argument("Use finite values between -100 and 100.");
                }
                result.push_back(value);
            }
            return result;
        };

        const auto values_a = read_vector(input_a, "Vector A");
        const auto values_b = (operation == 2) ? std::vector<double>{} : read_vector(input_b, "Vector B");

        EncryptionParameters parms(scheme_type::ckks);
        constexpr std::size_t poly_modulus_degree = 8192;
        parms.set_poly_modulus_degree(poly_modulus_degree);
        parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, { 60, 40, 40, 60 }));

        stage = "context creation";
        SEALContext context(parms);
        stage = "key generation";
        KeyGenerator keygen(context);
        SecretKey secret_key = keygen.secret_key();
        PublicKey public_key;
        keygen.create_public_key(public_key);

        RelinKeys relin_keys;
        GaloisKeys galois_keys;
        if (operation == 1)
        {
            stage = "relinearization key generation";
            keygen.create_relin_keys(relin_keys);
        }
        else if (operation == 2 && steps != 0)
        {
            stage = "rotation key generation";
            keygen.create_galois_keys(std::vector<int>{ steps }, galois_keys);
        }

        stage = "encoder setup";
        CKKSEncoder encoder(context);
        constexpr double scale = 1099511627776.0; // 2^40
        Plaintext plain_a;
        if (operation == 2)
        {
            std::vector<double> packed(encoder.slot_count());
            for (std::size_t i = 0; i < packed.size(); ++i)
            {
                packed[i] = values_a[i % values_a.size()];
            }
            encoder.encode(packed, scale, plain_a);
        }
        else
        {
            encoder.encode(values_a, scale, plain_a);
        }
        Ciphertext encrypted_a;
        Encryptor encryptor(context, public_key);
        encryptor.encrypt(plain_a, encrypted_a);

        Ciphertext result;
        Evaluator evaluator(context);
        if (operation == 2)
        {
            stage = "encrypted vector rotation";
            if (steps == 0)
            {
                result = encrypted_a;
            }
            else
            {
                evaluator.rotate_vector(encrypted_a, steps, galois_keys, result);
            }
        }
        else
        {
            Plaintext plain_b;
            encoder.encode(values_b, scale, plain_b);
            Ciphertext encrypted_b;
            encryptor.encrypt(plain_b, encrypted_b);
            if (operation == 0)
            {
                stage = "encrypted vector addition";
                evaluator.add(encrypted_a, encrypted_b, result);
            }
            else
            {
                stage = "encrypted vector multiplication";
                evaluator.multiply(encrypted_a, encrypted_b, result);
                evaluator.relinearize_inplace(result, relin_keys);
                evaluator.rescale_to_next_inplace(result);
            }
        }

        Plaintext decrypted;
        stage = "result decryption";
        Decryptor decryptor(context, secret_key);
        decryptor.decrypt(result, decrypted);
        std::vector<double> decoded;
        encoder.decode(decrypted, decoded);

        std::stringstream serialized;
        result.save(serialized);
        val output = val::object();
        val output_values = val::array();
        for (unsigned i = 0; i < 4; ++i)
        {
            output_values.set(i, decoded[i]);
        }
        output.set("values", output_values);
        output.set("ciphertextBytes", static_cast<double>(serialized.str().size()));
        return output;
    }
    catch (const std::exception &error)
    {
        val output = val::object();
        output.set("error", stage + ": " + error.what());
        return output;
    }
}

EMSCRIPTEN_BINDINGS(seal_demo)
{
    function("compute", &compute);
    function("computeCkks", &compute_ckks);
}
