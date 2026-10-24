#include <seal/seal.h>
#include <emscripten/bind.h>
#include <sstream>
#include <stdexcept>
#include <string>

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

EMSCRIPTEN_BINDINGS(seal_demo)
{
    function("compute", &compute);
}
