#include <sys/socket.h>
//#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <net/inet4>

int create_socket(std::string, BIO*);

int init_ssl(const std::string& dest_url)
{
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();
  ERR_load_BIO_strings();
  ERR_load_crypto_strings();

  auto* certbio = BIO_new(BIO_s_file());
  auto* outbio  = BIO_new_fp(stdout, BIO_NOCLOSE);

  if (SSL_library_init() < 0) {
      BIO_printf(outbio, "Could not initialize the OpenSSL library !\n");
  }
  printf("init_ssl() done\n");

  auto method = SSLv23_client_method();

  /* ---------------------------------------------------------- *
   * Try to create a new SSL context                            *
   * ---------------------------------------------------------- */
  auto* ctx = SSL_CTX_new(method);
  if (ctx == NULL) {
    BIO_printf(outbio, "Unable to create a new SSL context structure.\n");
  }

  /* ---------------------------------------------------------- *
   * Disabling SSLv2 will leave v3 and TSLv1 for negotiation    *
   * ---------------------------------------------------------- */
  SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

  /* ---------------------------------------------------------- *
   * Create new SSL connection state object                     *
   * ---------------------------------------------------------- */
  auto* ssl = SSL_new(ctx);

  /* ---------------------------------------------------------- *
   * Make the underlying TCP socket connection                  *
   * ---------------------------------------------------------- */
  int server = create_socket(dest_url, outbio);
  if (server != 0) {
    BIO_printf(outbio, "Successfully made the TCP connection to: %s.\n", dest_url.c_str());
  }

  /* ---------------------------------------------------------- *
   * Attach the SSL session to the socket descriptor            *
   * ---------------------------------------------------------- */
  SSL_set_fd(ssl, server);

  /* ---------------------------------------------------------- *
   * Try to SSL-connect here, returns 1 for success             *
   * ---------------------------------------------------------- */
  if (SSL_connect(ssl) != 1)
    BIO_printf(outbio, "Error: Could not build a SSL session to: %s.\n", dest_url.c_str());
  else
    BIO_printf(outbio, "Successfully enabled SSL/TLS session to: %s.\n", dest_url.c_str());

  /* ---------------------------------------------------------- *
   * Get the remote certificate into the X509 structure         *
   * ---------------------------------------------------------- */
  auto* cert = SSL_get_peer_certificate(ssl);
  if (cert == NULL)
    BIO_printf(outbio, "Error: Could not get a certificate from: %s.\n", dest_url.c_str());
  else
    BIO_printf(outbio, "Retrieved the server's certificate from: %s.\n", dest_url.c_str());

  /* ---------------------------------------------------------- *
   * extract various certificate information                    *
   * -----------------------------------------------------------*/
  auto* certname = X509_NAME_new();
  certname = X509_get_subject_name(cert);

  /* ---------------------------------------------------------- *
   * display the cert subject here                              *
   * -----------------------------------------------------------*/
  BIO_printf(outbio, "Displaying the certificate subject data:\n");
  X509_NAME_print_ex(outbio, certname, 0, 0);
  BIO_printf(outbio, "\n");

  /* ---------------------------------------------------------- *
   * Free the structures we don't need anymore                  *
   * -----------------------------------------------------------*/
  SSL_free(ssl);
  close(server);
  X509_free(cert);
  SSL_CTX_free(ctx);
  BIO_printf(outbio, "Finished SSL/TLS connection with server: %s.\n", dest_url.c_str());
  return 0;
}

/* ---------------------------------------------------------- *
* create_socket() creates the socket & TCP-connect to server *
* ---------------------------------------------------------- */
int create_socket(std::string url, BIO *out)
{
  net::IP4::addr addr("10.0.0.1");
  const uint16_t port = 443;
  /* ---------------------------------------------------------- *
   * create the basic TCP socket                                *
   * ---------------------------------------------------------- */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in dest_addr;
  dest_addr.sin_family=AF_INET;
  dest_addr.sin_port=htons(port);
  dest_addr.sin_addr.s_addr = addr.whole;

  /* ---------------------------------------------------------- *
   * Try to make the host connect here                          *
   * ---------------------------------------------------------- */
  if ( connect(sockfd, (struct sockaddr *) &dest_addr,
                              sizeof(struct sockaddr)) == -1 ) {
    BIO_printf(out, "Error: Cannot connect to host %s:%d.\n",
               addr.to_string().c_str(), port);
  }

  return sockfd;
}
