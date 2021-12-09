/*-
 * @file   genericCMPClient.h
 * @brief  generic CMP client library API
 *
 * @author David von Oheimb, Siemens AG, David.von.Oheimb@siemens.com
 *
 *  Copyright (c) 2017-2021 Siemens AG
 *  Licensed under the Apache License 2.0 (the "License").
 *  You may not use this file except in compliance with the License.
 *  You can obtain a copy in the file LICENSE in the source distribution
 *  or at https://www.openssl.org/source/license.html
 *  SPDX-License-Identifier: Apache-2.0
 */

#ifndef GENERIC_CMP_CLIENT_H
# define GENERIC_CMP_CLIENT_H

# ifdef CMP_STANDALONE
#  include <openssl/openssl_backport.h> /* needed for OpenSSL version < 3.0 */
# endif
/* for low-level CMP API, in particular, type OSSL_CMP_CTX */
# include <openssl/cmp.h>
/* for abbreviation and backward compatibility: */
typedef OSSL_CMP_CTX CMP_CTX;
typedef OSSL_CMP_severity severity;

# define CMPCLIENT_MODULE_NAME "genCMPClient"

typedef int CMP_err;
# define CMP_OK 0
# define CMP_R_OTHER_LIB_ERR 99
# define CMP_R_LOAD_CERTS   255
# define CMP_R_LOAD_CREDS   254
# define CMP_R_GENERATE_KEY 253
# define CMP_R_STORE_CREDS  252
# define CMP_R_RECIPIENT    251
# define CMP_R_INVALID_CONTEXT 250
# define CMP_R_INVALID_PARAMETERS CMP_R_INVALID_ARGS

/* further error codes are defined in ../cmpossl/include/openssl/cmperr.h */

# define CMP_IR    0 /* OSSL_CMP_PKIBODY_IR */
# define CMP_CR    2 /* OSSL_CMP_PKIBODY_CR */
# define CMP_P10CR 4 /* OSSL_CMP_PKIBODY_P10CR */
# define CMP_KUR   7 /* OSSL_CMP_PKIBODY_KUR */
# define CMP_RR   11 /* OSSL_CMP_PKIBODY_RR */

/* # define LOCAL_DEFS */
# ifdef LOCAL_DEFS
#  include "genericCMPClient_imports.h"
# else
#  include <secutils/credentials/credentials.h>
#  include <secutils/util/log.h>
# endif

/* CMP client core functions */
/* should be called once, as soon as the application starts */
CMP_err CMPclient_init(OPTIONAL const char* name, OPTIONAL LOG_cb_t log_fn);

/* must be called first */
CMP_err CMPclient_prepare(CMP_CTX **pctx,
                          OPTIONAL LOG_cb_t log_fn,
                          OPTIONAL X509_STORE *cmp_truststore,
                          OPTIONAL const char *recipient,
                          OPTIONAL const STACK_OF(X509) *untrusted,
                          OPTIONAL const CREDENTIALS *creds,
                          OPTIONAL X509_STORE *creds_truststore,
                          OPTIONAL const char *digest,
                          OPTIONAL const char *mac,
                          OPTIONAL OSSL_CMP_transfer_cb_t transfer_fn,
                          int total_timeout,
                          OPTIONAL X509_STORE *new_cert_truststore,
                          bool implicit_confirm);

/* must be called next if the transfer_fn is NULL and no existing connection is used */
/* Will return error when used with OpenSSL compiled with OPENSSL_NO_SOCK. */
CMP_err CMPclient_setup_HTTP(CMP_CTX *ctx, const char *server, const char *path,
                             int keep_alive, int timeout, OPTIONAL SSL_CTX *tls,
                             OPTIONAL const char *proxy,
                             OPTIONAL const char *no_proxy);
/* must be called alternatively if transfer_fn is NULL and existing connection is used */
CMP_err CMPclient_setup_BIO(CMP_CTX *ctx, BIO *rw, const char *path,
                            int keep_alive, int timeout);

/*-
 * @brief fill the cert template used for certificate requests (ir/cr/p10cr/kur)
 *
 * @param |ctx| CMP context to be used for implicit parameters, may get updated
 * @param |new_key| key pair to be certified; defaults to key in |csr| or |creds->key|
 * @param |old_cert| reference cert to be updated (kur), defaults to data in |csr| or |creds->cert|
 * @param |subject| to use; defaults to subject of |csr| or of reference cert for KUR,
 *        while this default is is not used for IR and CR if the |exts| or |csr| contain SANs.
 * @param |exts| X.509v3 extensions to use.
 *        Extensions provided via the |csr| parameter are augmented or overridden individually.
 *        SANs default to SANs contained in the reference cert |old_cert|.
 * @param |csr| PKCS#10 structure directly used for P10CR command, else its contents are transformed.
 * @note All 'const' parameters are copied (and need to be freed by the caller).
 * @return CMP_OK on success, else CMP error code
 */
CMP_err CMPclient_setup_certreq(CMP_CTX *ctx,
                                OPTIONAL const EVP_PKEY *new_key,
                                OPTIONAL const X509 *old_cert,
                                OPTIONAL const X509_NAME *subject,
                                OPTIONAL const X509_EXTENSIONS *exts,
                                OPTIONAL const X509_REQ *csr);

/*-
 * Either the internal CMPclient_enroll() or the specific CMPclient_imprint(),
 * CMPclient_bootstrap(), CMPclient_pkcs10(), or CMPclient_update[_anycert]())
 * or CMPclient_revoke() can be called next.
 */
/* the structure returned in *new_creds must be freed by the caller */
/*
 * @brief perform the given type of certificate request (ir/cr/p10cr/kur)
 *
 * @param |ctx| CMP context to be used for implicit parameters, may get updated
 * @param |new_creds| pointer to variable where to store new credentials including enrolled certificate
 * @param |cmd| the type of request to be performed: CMP_IR, CMP_CR, CMP_P10CR, or CMP_KUR
 * @return CMP_OK on success, else CMP error code
 */
CMP_err CMPclient_enroll(CMP_CTX *ctx, CREDENTIALS **new_creds, int cmd);
CMP_err CMPclient_imprint(CMP_CTX *ctx, CREDENTIALS **new_creds,
                          const EVP_PKEY *new_key, const char *subject,
                          OPTIONAL const X509_EXTENSIONS *exts);
CMP_err CMPclient_bootstrap(CMP_CTX *ctx, CREDENTIALS **new_creds,
                            const EVP_PKEY *new_key, const char *subject,
                            OPTIONAL const X509_EXTENSIONS *exts);
CMP_err CMPclient_pkcs10(CMP_CTX *ctx, CREDENTIALS **new_creds,
                         const X509_REQ *csr);
CMP_err CMPclient_update(CMP_CTX *ctx, CREDENTIALS **new_creds,
                         const EVP_PKEY *new_key);
CMP_err CMPclient_update_anycert(OSSL_CMP_CTX *ctx, CREDENTIALS **new_creds,
                                 OPTIONAL const X509 *old_cert, const EVP_PKEY *new_key);

/* reason codes are defined in openssl/x509v3.h */
CMP_err CMPclient_revoke(CMP_CTX *ctx, const X509 *cert, /* TODO: X509_REQ *csr, */ int reason);

/* get error information sent by the server */
char *CMPclient_snprint_PKIStatus(const OSSL_CMP_CTX *ctx, char *buf, size_t bufsize);

/* must be called between any of the above certificate management activities */
CMP_err CMPclient_reinit(CMP_CTX *ctx);

/* should be called on application termination */
void CMPclient_finish(OPTIONAL CMP_CTX *ctx);

/* CREDENTIALS helpers */
# ifdef LOCAL_DEFS
#  include "genericCMPClient_imports.h"
# else
#  include <secutils/credentials/key.h>
# endif

/* X509_STORE helpers */
EVP_PKEY *KEY_load(OPTIONAL const char *file, OPTIONAL const char *pass,
                   OPTIONAL const char *engine, OPTIONAL const char *desc);
X509_REQ *CSR_load(const char *file, OPTIONAL const char *desc);

STACK_OF(X509_CRL) *CRLs_load(const char *files, int timeout, OPTIONAL const char *desc);
void CRLs_free(OPTIONAL STACK_OF(X509_CRL) *crls);
X509_STORE *STORE_load(const char *trusted_certs, OPTIONAL const char *desc,
                       OPTIONAL X509_VERIFY_PARAM *vpm);
# ifdef LOCAL_DEFS
#  include "genericCMPClient_imports.h"
# else
#  include <secutils/credentials/store.h>
# endif

/* SSL_CTX helpers for HTTPS */
# ifndef SEC_NO_TLS
#  ifdef LOCAL_DEFS
#   include "genericCMPClient_imports.h"
#  else
#   include <secutils/connections/tls.h>
#  endif
SSL_CTX *TLS_new(OPTIONAL const X509_STORE *truststore,
                 OPTIONAL const STACK_OF(X509) *untrusted,
                 OPTIONAL const CREDENTIALS *creds,
                 OPTIONAL const char *ciphers, int security_level);
void TLS_free(OPTIONAL SSL_CTX *tls);
# endif

/* X509_EXTENSIONS helpers */
# ifdef LOCAL_DEFS
#  include "genericCMPClient_imports.h"
# else
#  include <secutils/util/extensions.h>
# endif

#endif /* GENERIC_CMP_CLIENT_H */
