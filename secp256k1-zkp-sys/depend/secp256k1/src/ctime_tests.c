/***********************************************************************
 * Copyright (c) 2020 Gregory Maxwell                                  *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include <stdio.h>
#include <string.h>

#include "../include/secp256k1.h"
#include "assumptions.h"
#include "checkmem.h"

#if !SECP256K1_CHECKMEM_ENABLED
#  error "This tool cannot be compiled without memory-checking interface (valgrind or msan)"
#endif

#ifdef ENABLE_MODULE_ECDH
#include "../include/secp256k1_ecdh.h"
#endif

#ifdef ENABLE_MODULE_RECOVERY
#include "../include/secp256k1_recovery.h"
#endif

#ifdef ENABLE_MODULE_EXTRAKEYS
#include "../include/secp256k1_extrakeys.h"
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
#include "../include/secp256k1_schnorrsig.h"
#endif

#ifdef ENABLE_MODULE_ELLSWIFT
#include "../include/secp256k1_ellswift.h"
#endif

#ifdef ENABLE_MODULE_ECDSA_S2C
#include "../include/secp256k1_ecdsa_s2c.h"
#endif

#ifdef ENABLE_MODULE_ECDSA_ADAPTOR
#include "../include/secp256k1_ecdsa_adaptor.h"
#endif

#ifdef ENABLE_MODULE_MUSIG
#include "../include/secp256k1_musig.h"
#endif

#ifdef ENABLE_MODULE_FROST
#include "include/secp256k1_frost.h"
#endif

static void run_tests(rustsecp256k1zkp_v0_10_0_context *ctx, unsigned char *key);

int main(void) {
    rustsecp256k1zkp_v0_10_0_context* ctx;
    unsigned char key[32];
    int ret, i;

    if (!SECP256K1_CHECKMEM_RUNNING()) {
        fprintf(stderr, "This test can only usefully be run inside valgrind because it was not compiled under msan.\n");
        fprintf(stderr, "Usage: libtool --mode=execute valgrind ./ctime_tests\n");
        return 1;
    }
    ctx = rustsecp256k1zkp_v0_10_0_context_create(SECP256K1_CONTEXT_DECLASSIFY);
    /** In theory, testing with a single secret input should be sufficient:
     *  If control flow depended on secrets the tool would generate an error.
     */
    for (i = 0; i < 32; i++) {
        key[i] = i + 65;
    }

    run_tests(ctx, key);

    /* Test context randomisation. Do this last because it leaves the context
     * tainted. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_context_randomize(ctx, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);

    rustsecp256k1zkp_v0_10_0_context_destroy(ctx);
    return 0;
}

static void run_tests(rustsecp256k1zkp_v0_10_0_context *ctx, unsigned char *key) {
    rustsecp256k1zkp_v0_10_0_ecdsa_signature signature;
    rustsecp256k1zkp_v0_10_0_pubkey pubkey;
    size_t siglen = 74;
    size_t outputlen = 33;
    int i;
    int ret;
    unsigned char msg[32];
    unsigned char sig[74];
    unsigned char spubkey[33];
#ifdef ENABLE_MODULE_RECOVERY
    rustsecp256k1zkp_v0_10_0_ecdsa_recoverable_signature recoverable_signature;
    int recid;
#endif
#ifdef ENABLE_MODULE_EXTRAKEYS
    rustsecp256k1zkp_v0_10_0_keypair keypair;
#endif
#ifdef ENABLE_MODULE_ELLSWIFT
    unsigned char ellswift[64];
    static const unsigned char prefix[64] = {'t', 'e', 's', 't'};
#endif

    for (i = 0; i < 32; i++) {
        msg[i] = i + 1;
    }

    /* Test keygen. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_ec_pubkey_create(ctx, &pubkey, key);
    SECP256K1_CHECKMEM_DEFINE(&pubkey, sizeof(rustsecp256k1zkp_v0_10_0_pubkey));
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(rustsecp256k1zkp_v0_10_0_ec_pubkey_serialize(ctx, spubkey, &outputlen, &pubkey, SECP256K1_EC_COMPRESSED) == 1);

    /* Test signing. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_ecdsa_sign(ctx, &signature, msg, key, NULL, NULL);
    SECP256K1_CHECKMEM_DEFINE(&signature, sizeof(rustsecp256k1zkp_v0_10_0_ecdsa_signature));
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(rustsecp256k1zkp_v0_10_0_ecdsa_signature_serialize_der(ctx, sig, &siglen, &signature));

#ifdef ENABLE_MODULE_ECDH
    /* Test ECDH. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_ecdh(ctx, msg, &pubkey, key, NULL, NULL);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_RECOVERY
    /* Test signing a recoverable signature. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_ecdsa_sign_recoverable(ctx, &recoverable_signature, msg, key, NULL, NULL);
    SECP256K1_CHECKMEM_DEFINE(&recoverable_signature, sizeof(recoverable_signature));
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(rustsecp256k1zkp_v0_10_0_ecdsa_recoverable_signature_serialize_compact(ctx, sig, &recid, &recoverable_signature));
    CHECK(recid >= 0 && recid <= 3);
#endif

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_ec_seckey_verify(ctx, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_ec_seckey_negate(ctx, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    SECP256K1_CHECKMEM_UNDEFINE(msg, 32);
    ret = rustsecp256k1zkp_v0_10_0_ec_seckey_tweak_add(ctx, key, msg);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    SECP256K1_CHECKMEM_UNDEFINE(msg, 32);
    ret = rustsecp256k1zkp_v0_10_0_ec_seckey_tweak_mul(ctx, key, msg);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    /* Test keypair_create and keypair_xonly_tweak_add. */
#ifdef ENABLE_MODULE_EXTRAKEYS
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_keypair_create(ctx, &keypair, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    /* The tweak is not treated as a secret in keypair_tweak_add */
    SECP256K1_CHECKMEM_DEFINE(msg, 32);
    ret = rustsecp256k1zkp_v0_10_0_keypair_xonly_tweak_add(ctx, &keypair, msg);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    SECP256K1_CHECKMEM_UNDEFINE(&keypair, sizeof(keypair));
    ret = rustsecp256k1zkp_v0_10_0_keypair_sec(ctx, key, &keypair);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_keypair_create(ctx, &keypair, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
    ret = rustsecp256k1zkp_v0_10_0_schnorrsig_sign32(ctx, sig, msg, &keypair, NULL);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_ELLSWIFT
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_ellswift_create(ctx, ellswift, key, NULL);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = rustsecp256k1zkp_v0_10_0_ellswift_create(ctx, ellswift, key, ellswift);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    for (i = 0; i < 2; i++) {
        SECP256K1_CHECKMEM_UNDEFINE(key, 32);
        SECP256K1_CHECKMEM_DEFINE(&ellswift, sizeof(ellswift));
        ret = rustsecp256k1zkp_v0_10_0_ellswift_xdh(ctx, msg, ellswift, ellswift, key, i, rustsecp256k1zkp_v0_10_0_ellswift_xdh_hash_function_bip324, NULL);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);

        SECP256K1_CHECKMEM_UNDEFINE(key, 32);
        SECP256K1_CHECKMEM_DEFINE(&ellswift, sizeof(ellswift));
        ret = rustsecp256k1zkp_v0_10_0_ellswift_xdh(ctx, msg, ellswift, ellswift, key, i, rustsecp256k1zkp_v0_10_0_ellswift_xdh_hash_function_prefix, (void *)prefix);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
    }
#endif

#ifdef ENABLE_MODULE_ECDSA_S2C
    {
        unsigned char s2c_data[32] = {0};
        unsigned char s2c_data_comm[32] = {0};
        rustsecp256k1zkp_v0_10_0_ecdsa_s2c_opening s2c_opening;

        SECP256K1_CHECKMEM_UNDEFINE(key, 32);
        SECP256K1_CHECKMEM_UNDEFINE(s2c_data, 32);
        ret = rustsecp256k1zkp_v0_10_0_ecdsa_s2c_sign(ctx, &signature, &s2c_opening, msg, key, s2c_data);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);

        SECP256K1_CHECKMEM_UNDEFINE(s2c_data, 32);
        ret = rustsecp256k1zkp_v0_10_0_ecdsa_anti_exfil_host_commit(ctx, s2c_data_comm, s2c_data);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);

        SECP256K1_CHECKMEM_UNDEFINE(key, 32);
        SECP256K1_CHECKMEM_UNDEFINE(s2c_data, 32);
        ret = rustsecp256k1zkp_v0_10_0_ecdsa_anti_exfil_signer_commit(ctx, &s2c_opening, msg, key, s2c_data);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
    }
#endif

#ifdef ENABLE_MODULE_ECDSA_ADAPTOR
    {
        unsigned char adaptor_sig[162];
        unsigned char deckey[32];
        unsigned char expected_deckey[32];
        rustsecp256k1zkp_v0_10_0_pubkey enckey;

        for (i = 0; i < 32; i++) {
            deckey[i] = i + 2;
        }

        ret = rustsecp256k1zkp_v0_10_0_ec_pubkey_create(ctx, &enckey, deckey);
        CHECK(ret == 1);

        SECP256K1_CHECKMEM_UNDEFINE(key, 32);
        ret = rustsecp256k1zkp_v0_10_0_ecdsa_adaptor_encrypt(ctx, adaptor_sig, key, &enckey, msg, NULL, NULL);
        SECP256K1_CHECKMEM_DEFINE(adaptor_sig, sizeof(adaptor_sig));
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);

        SECP256K1_CHECKMEM_UNDEFINE(deckey, 32);
        ret = rustsecp256k1zkp_v0_10_0_ecdsa_adaptor_decrypt(ctx, &signature, deckey, adaptor_sig);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);

        SECP256K1_CHECKMEM_UNDEFINE(&signature, 32);
        ret = rustsecp256k1zkp_v0_10_0_ecdsa_adaptor_recover(ctx, expected_deckey, &signature, adaptor_sig, &enckey);
        SECP256K1_CHECKMEM_DEFINE(expected_deckey, sizeof(expected_deckey));
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);

        SECP256K1_CHECKMEM_DEFINE(deckey, sizeof(deckey));
        ret = rustsecp256k1zkp_v0_10_0_memcmp_var(deckey, expected_deckey, sizeof(expected_deckey));
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 0);
    }
#endif

#ifdef ENABLE_MODULE_MUSIG
    {
        rustsecp256k1zkp_v0_10_0_pubkey pk;
        const rustsecp256k1zkp_v0_10_0_pubkey *pk_ptr[1];
        rustsecp256k1zkp_v0_10_0_xonly_pubkey agg_pk;
        unsigned char session_id[32];
        rustsecp256k1zkp_v0_10_0_musig_secnonce secnonce;
        rustsecp256k1zkp_v0_10_0_musig_pubnonce pubnonce;
        const rustsecp256k1zkp_v0_10_0_musig_pubnonce *pubnonce_ptr[1];
        rustsecp256k1zkp_v0_10_0_musig_aggnonce aggnonce;
        rustsecp256k1zkp_v0_10_0_musig_keyagg_cache cache;
        rustsecp256k1zkp_v0_10_0_musig_session session;
        rustsecp256k1zkp_v0_10_0_musig_partial_sig partial_sig;
        const rustsecp256k1zkp_v0_10_0_musig_partial_sig *partial_sig_ptr[1];
        unsigned char extra_input[32];
        unsigned char sec_adaptor[32];
        rustsecp256k1zkp_v0_10_0_pubkey adaptor;
        unsigned char pre_sig[64];
        int nonce_parity;

        pk_ptr[0] = &pk;
        pubnonce_ptr[0] = &pubnonce;
        SECP256K1_CHECKMEM_DEFINE(key, 32);
        memcpy(session_id, key, sizeof(session_id));
        session_id[0] = session_id[0] + 1;
        memcpy(extra_input, key, sizeof(extra_input));
        extra_input[0] = extra_input[0] + 2;
        memcpy(sec_adaptor, key, sizeof(sec_adaptor));
        sec_adaptor[0] = extra_input[0] + 3;
        partial_sig_ptr[0] = &partial_sig;

        CHECK(rustsecp256k1zkp_v0_10_0_keypair_create(ctx, &keypair, key));
        CHECK(rustsecp256k1zkp_v0_10_0_keypair_pub(ctx, &pk, &keypair));
        CHECK(rustsecp256k1zkp_v0_10_0_musig_pubkey_agg(ctx, NULL, &agg_pk, &cache, pk_ptr, 1));
        CHECK(rustsecp256k1zkp_v0_10_0_ec_pubkey_create(ctx, &adaptor, sec_adaptor));
        SECP256K1_CHECKMEM_UNDEFINE(key, 32);
        SECP256K1_CHECKMEM_UNDEFINE(session_id, sizeof(session_id));
        SECP256K1_CHECKMEM_UNDEFINE(extra_input, sizeof(extra_input));
        SECP256K1_CHECKMEM_UNDEFINE(sec_adaptor, sizeof(sec_adaptor));
        ret = rustsecp256k1zkp_v0_10_0_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_id, key, &pk, msg, &cache, extra_input);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        CHECK(rustsecp256k1zkp_v0_10_0_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptr, 1));
        /* Make sure that previous tests don't undefine msg. It's not used as a secret here. */
        SECP256K1_CHECKMEM_DEFINE(msg, sizeof(msg));
        CHECK(rustsecp256k1zkp_v0_10_0_musig_nonce_process(ctx, &session, &aggnonce, msg, &cache, &adaptor) == 1);

        ret = rustsecp256k1zkp_v0_10_0_keypair_create(ctx, &keypair, key);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        ret = rustsecp256k1zkp_v0_10_0_musig_partial_sign(ctx, &partial_sig, &secnonce, &keypair, &cache, &session);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);

        SECP256K1_CHECKMEM_DEFINE(&partial_sig, sizeof(partial_sig));
        CHECK(rustsecp256k1zkp_v0_10_0_musig_partial_sig_agg(ctx, pre_sig, &session, partial_sig_ptr, 1));
        SECP256K1_CHECKMEM_DEFINE(pre_sig, sizeof(pre_sig));

        CHECK(rustsecp256k1zkp_v0_10_0_musig_nonce_parity(ctx, &nonce_parity, &session));
        ret = rustsecp256k1zkp_v0_10_0_musig_adapt(ctx, sig, pre_sig, sec_adaptor, nonce_parity);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        ret = rustsecp256k1zkp_v0_10_0_musig_extract_adaptor(ctx, sec_adaptor, sig, pre_sig, nonce_parity);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
    }
#endif

#ifdef ENABLE_MODULE_FROST
    {
        rustsecp256k1zkp_v0_10_0_pubkey pk[2];
        unsigned char session_id[32];
        unsigned char seed[2][32];
        rustsecp256k1zkp_v0_10_0_frost_secnonce secnonce[2];
        rustsecp256k1zkp_v0_10_0_frost_pubnonce pubnonce[2];
        const rustsecp256k1zkp_v0_10_0_frost_pubnonce *pubnonce_ptr[2];
        rustsecp256k1zkp_v0_10_0_frost_keygen_cache cache;
        rustsecp256k1zkp_v0_10_0_frost_session session;
        rustsecp256k1zkp_v0_10_0_frost_partial_sig partial_sig;
        const rustsecp256k1zkp_v0_10_0_frost_partial_sig *partial_sig_ptr[1];
        unsigned char extra_input[32];
        unsigned char sec_adaptor[32];
        rustsecp256k1zkp_v0_10_0_pubkey adaptor;
        unsigned char pre_sig[64];
        int nonce_parity;
        rustsecp256k1zkp_v0_10_0_frost_share shares[2][2];
        const rustsecp256k1zkp_v0_10_0_frost_share *share_ptr[2];
        rustsecp256k1zkp_v0_10_0_frost_share agg_share;
        const rustsecp256k1zkp_v0_10_0_pubkey *vss_ptr[2];
        unsigned char pok[2][64];
        rustsecp256k1zkp_v0_10_0_pubkey vss_commitment[2][2];
        unsigned char key2[32];
        rustsecp256k1zkp_v0_10_0_keypair keypair2;
        unsigned char id[2][33];
        const unsigned char *id_ptr[2];
        size_t size = 33;
        const unsigned char *pok_ptr[2];
        rustsecp256k1zkp_v0_10_0_pubkey pubshare[2];
        const rustsecp256k1zkp_v0_10_0_pubkey *pubshares_ptr[2];

        id_ptr[0] = id[0];
        id_ptr[1] = id[1];
        pubnonce_ptr[0] = &pubnonce[0];
        pubnonce_ptr[1] = &pubnonce[1];
        SECP256K1_CHECKMEM_DEFINE(key, 32);
        memcpy(seed[0], key, 32);
        seed[0][0] = seed[0][0] + 1;
        memcpy(seed[0], key, 32);
        seed[1][0] = seed[1][0] + 2;
        memcpy(extra_input, key, sizeof(extra_input));
        extra_input[0] = extra_input[0] + 3;
        memcpy(sec_adaptor, key, sizeof(sec_adaptor));
        sec_adaptor[0] = extra_input[0] + 4;
        memcpy(key2, key, sizeof(key2));
        key2[0] = key2[0] + 5;
        memcpy(session_id, key, sizeof(session_id));
        session_id[0] = session_id[0] + 6;
        partial_sig_ptr[0] = &partial_sig;
        share_ptr[0] = &shares[0][0];
        share_ptr[1] = &shares[1][0];
        vss_ptr[0] = vss_commitment[0];
        vss_ptr[1] = vss_commitment[1];
        pok_ptr[0] = pok[0];
        pok_ptr[1] = pok[1];
        pubshares_ptr[0] = &pubshare[0];
        pubshares_ptr[1] = &pubshare[1];

        CHECK(rustsecp256k1zkp_v0_10_0_keypair_create(ctx, &keypair, key));
        CHECK(rustsecp256k1zkp_v0_10_0_keypair_create(ctx, &keypair2, key2));
        CHECK(rustsecp256k1zkp_v0_10_0_keypair_pub(ctx, &pk[0], &keypair));
        CHECK(rustsecp256k1zkp_v0_10_0_keypair_pub(ctx, &pk[1], &keypair2));
        CHECK(rustsecp256k1zkp_v0_10_0_ec_pubkey_serialize(ctx, id[0], &size, &pk[0], SECP256K1_EC_COMPRESSED));
        CHECK(rustsecp256k1zkp_v0_10_0_ec_pubkey_serialize(ctx, id[1], &size, &pk[1], SECP256K1_EC_COMPRESSED));

        /* shares_gen */
        SECP256K1_CHECKMEM_UNDEFINE(seed[0], 32);
        SECP256K1_CHECKMEM_UNDEFINE(seed[1], 32);
        ret = rustsecp256k1zkp_v0_10_0_frost_shares_gen(ctx, shares[0], vss_commitment[0], pok[0], seed[0], 2, 2, id_ptr);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        ret = rustsecp256k1zkp_v0_10_0_frost_shares_gen(ctx, shares[1], vss_commitment[1], pok[1], seed[1], 2, 2, id_ptr);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        SECP256K1_CHECKMEM_UNDEFINE(&shares[0][0], sizeof(shares[0][0]));
        SECP256K1_CHECKMEM_UNDEFINE(&shares[1][0], sizeof(shares[1][0]));
        /* share_agg */
        SECP256K1_CHECKMEM_DEFINE(&vss_commitment[0][0], sizeof(rustsecp256k1zkp_v0_10_0_pubkey));
        SECP256K1_CHECKMEM_DEFINE(&vss_commitment[0][1], sizeof(rustsecp256k1zkp_v0_10_0_pubkey));
        SECP256K1_CHECKMEM_DEFINE(&vss_commitment[1][0], sizeof(rustsecp256k1zkp_v0_10_0_pubkey));
        SECP256K1_CHECKMEM_DEFINE(&vss_commitment[1][1], sizeof(rustsecp256k1zkp_v0_10_0_pubkey));
        SECP256K1_CHECKMEM_DEFINE(pok[0], 64);
        SECP256K1_CHECKMEM_DEFINE(pok[1], 64);
        ret = rustsecp256k1zkp_v0_10_0_frost_share_agg(ctx, &agg_share, share_ptr, vss_ptr, pok_ptr, 2, 2, id_ptr[0]);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        SECP256K1_CHECKMEM_UNDEFINE(&agg_share, sizeof(&agg_share));
        CHECK(rustsecp256k1zkp_v0_10_0_frost_compute_pubshare(ctx, &pubshare[0], 2, id_ptr[0], vss_ptr, 2));
        CHECK(rustsecp256k1zkp_v0_10_0_frost_compute_pubshare(ctx, &pubshare[1], 2, id_ptr[1], vss_ptr, 2));
        CHECK(rustsecp256k1zkp_v0_10_0_frost_pubkey_gen(ctx, &cache, pubshares_ptr, 2, id_ptr));
        /* nonce_gen */
        SECP256K1_CHECKMEM_UNDEFINE(session_id, sizeof(session_id));
        CHECK(rustsecp256k1zkp_v0_10_0_ec_pubkey_create(ctx, &adaptor, sec_adaptor));
        SECP256K1_CHECKMEM_UNDEFINE(extra_input, sizeof(extra_input));
        SECP256K1_CHECKMEM_UNDEFINE(sec_adaptor, sizeof(sec_adaptor));
        ret = rustsecp256k1zkp_v0_10_0_frost_nonce_gen(ctx, &secnonce[0], &pubnonce[0], session_id, &agg_share, msg, &cache, extra_input);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        ret = rustsecp256k1zkp_v0_10_0_frost_nonce_gen(ctx, &secnonce[1], &pubnonce[1], session_id, &agg_share, msg, &cache, extra_input);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        /* partial_sign */
        /* Make sure that previous tests don't undefine msg. It's not used as a secret here. */
        SECP256K1_CHECKMEM_DEFINE(msg, sizeof(msg));
        CHECK(rustsecp256k1zkp_v0_10_0_frost_nonce_process(ctx, &session, pubnonce_ptr, 2, msg, id_ptr[0], id_ptr, &cache, &adaptor) == 1);
        ret = rustsecp256k1zkp_v0_10_0_frost_partial_sign(ctx, &partial_sig, &secnonce[0], &agg_share, &session, &cache);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        /* adapt */
        SECP256K1_CHECKMEM_DEFINE(&partial_sig, sizeof(partial_sig));
        CHECK(rustsecp256k1zkp_v0_10_0_frost_partial_sig_agg(ctx, pre_sig, &session, partial_sig_ptr, 1));
        SECP256K1_CHECKMEM_DEFINE(pre_sig, sizeof(pre_sig));
        CHECK(rustsecp256k1zkp_v0_10_0_frost_nonce_parity(ctx, &nonce_parity, &session));
        ret = rustsecp256k1zkp_v0_10_0_frost_adapt(ctx, sig, pre_sig, sec_adaptor, nonce_parity);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        /* extract_adaptor */
        ret = rustsecp256k1zkp_v0_10_0_frost_extract_adaptor(ctx, sec_adaptor, sig, pre_sig, nonce_parity);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1); 
    }
#endif
}
