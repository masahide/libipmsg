/**
 * IP ��å��󥸥�饤�֥��(Unix��)
 * IP��å��󥸥㥨��������ȥ��饹�ΰŹ��Ϣ�᥽�åɷ���
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "IpMessenger.h"
#include "IpMessengerImpl.h"
#include "ipmsg.h"

using namespace ipmsg;

#define WINCOMPAT

#ifdef HAVE_OPENSSL
#ifdef WINCOMPAT
#define SUPPORT_RSA_512
#define SUPPORT_RSA_1024
#define SUPPORT_RC2_40
#define SUPPORT_BLOWFISH_128
#else	// WINCOMPAT
#define SUPPORT_RSA_512
#define SUPPORT_RSA_1024
#define SUPPORT_RSA_2048
#define SUPPORT_RC2_40
#define SUPPORT_RC2_128
#define SUPPORT_RC2_256
#define SUPPORT_BLOWFISH_128
#define SUPPORT_BLOWFISH_256
#endif	// WINCOMPAT
#endif	// HAVE_OPENSSL

#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#endif	// HAVE_OPENSSL

//�Ź沽����(RSA)�Υӥåȿ�(�Ǽ�)
#define RSA_KEY_LENGTH_MINIMUM	512
//�Ź沽����(RSA)�Υӥåȿ�(�ޤ��ޤ�)
#define RSA_KEY_LENGTH_MIDIUM	1024
//�Ź沽����(RSA)�Υӥåȿ�(�Ƕ�)
#define RSA_KEY_LENGTH_MAXIMUM	2048

//RSA�����������˻��Ѥ����ǿ�
#define ENCRYPT_PRIME			65537

#define ERR_BUF_SIZE	1024
/**
 * �Ź��Ϣ�ν������
 * �������Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgentImpl::CryptoInit()
{
	IPMSG_FUNC_ENTER( "void IpMessengerAgentImpl::CryptoInit()" );
#ifdef HAVE_OPENSSL
	ERR_load_crypto_strings();

	encryptionCapacity = 0UL;
	RsaMax = NULL;
#ifdef SUPPORT_RSA_2048
	RsaMax = RSA_generate_key( RSA_KEY_LENGTH_MAXIMUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMax == NULL ) {
		char errbuf[ERR_BUF_SIZE];
		printf("IpMessengerAgentImpl::CryptoInit In Encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );fflush(stdout);
	} else {
		encryptionCapacity |= IPMSG_RSA_2048;
		printf("Encryption extention enabled.(RSA2048)\n");fflush(stdout);
	}
#endif	//SUPPORT_RSA_2048
	RsaMid = NULL;
#ifdef SUPPORT_RSA_1024
	RsaMid = RSA_generate_key( RSA_KEY_LENGTH_MIDIUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMid == NULL ) {
		char errbuf[ERR_BUF_SIZE];
		printf("IpMessengerAgentImpl::CryptoInit In Encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );fflush(stdout);
	} else {
		encryptionCapacity |= IPMSG_RSA_1024;
		printf("Encryption extention enabled.(RSA1024)\n");fflush(stdout);
	}
#endif	//SUPPORT_RSA_1024
	RsaMin = NULL;
#ifdef SUPPORT_RSA_512
	RsaMin = RSA_generate_key( RSA_KEY_LENGTH_MINIMUM, ENCRYPT_PRIME, NULL, NULL );
	if ( RsaMin == NULL ) {
		char errbuf[ERR_BUF_SIZE];
		printf("IpMessengerAgentImpl::CryptoInit In Encrypt: err=%s\n", ERR_error_string( ERR_get_error(), errbuf ) );fflush(stdout);
	} else {
		encryptionCapacity |= IPMSG_RSA_512;
		printf("Encryption extention enabled.(RSA512)\n");fflush(stdout);
	}
#endif	//SUPPORT_RSA_512
	if ( encryptionCapacity == 0UL ) {
		//�Ź沽̵��
		printf("Encryption extention disabled.\n");fflush(stdout);
	}
#ifdef SUPPORT_RC2_40
	encryptionCapacity |= IPMSG_RC2_40;
#endif	//SUPPORT_RC2_40
#ifdef SUPPORT_RC2_128
	encryptionCapacity |= IPMSG_RC2_128;
#endif	//SUPPORT_RC2_128

#ifdef SUPPORT_RC2_256
	encryptionCapacity |= IPMSG_RC2_256;
#endif	//SUPPORT_RC2_256

#ifdef SUPPORT_BLOWFISH_128
	encryptionCapacity |= IPMSG_BLOWFISH_128;
#endif	//SUPPORT_BLOWFISH_128

#ifdef SUPPORT_BLOWFISH_256
	encryptionCapacity |= IPMSG_BLOWFISH_256;
#endif	//SUPPORT_BLOWFISH_256
#endif	//HAVE_OPENSSL
	IPMSG_FUNC_EXIT;
}

/**
 * �Ź��Ϣ�ν�������
 * �������Υ᥽�åɤϥ���åɥ����դǤʤ���
 */
void
IpMessengerAgentImpl::CryptoEnd()
{
	IPMSG_FUNC_ENTER( "void IpMessengerAgentImpl::CryptoEnd()" );
#ifdef HAVE_OPENSSL
	if ( RsaMin != NULL ) {
		RSA_free( RsaMin );
	}
	if ( RsaMid != NULL ) {
		RSA_free( RsaMid );
	}
	if ( RsaMax != NULL ) {
		RSA_free( RsaMax );
	}
	ERR_free_strings();
#endif	//HAVE_OPENSSL
	IPMSG_FUNC_EXIT;
}

/**
 * ��å������Ź沽��
 * @param host ������ۥ���
 * @param optBuf �ѥ��åȥ��ץ�������ΥХåե��Υ��ɥ쥹
 * @param optBufLen �ѥ��åȥ��ץ�������ΥХåե��θ��ߤ�ͭ���ǡ���Ĺ
 * @param enc_optBufLen �Ź沽�ѤΥѥ��åȥ��ץ�������ΥХåե���ͭ���ǡ���Ĺ�Υ��ɥ쥹
 * @param opt_size �ѥ��åȥ��ץ�������ΥХåե��Υ�����
 * @retval true:���沽����
 * @retval false:���沽����
 */
bool
IpMessengerAgentImpl::EncryptMsg( const HostListItem& host, unsigned char *optBuf, int optBufLen, int *enc_optBufLen, int opt_size )
{
	IPMSG_FUNC_ENTER( "bool IpMessengerAgentImpl::EncryptMsg( const HostListItem& host, unsigned char *optBuf, int optBufLen, int *enc_optBufLen, int opt_size )" );
#ifdef HAVE_OPENSSL
	unsigned long pubKeyMethod = 0UL;
	unsigned char iv[EVP_MAX_IV_LENGTH];

	//EVP��Seal�Ϥθ������Ź�ΰŹ沽API�ϻȤ��ˤ����Τǡ���ʬ��Encrypt�ϡ�RSA�Ϥ�API�Ǽ������ޤ���
#ifndef WINCOMPAT
#ifdef SUPPORT_RSA_2048
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_2048 && host.EncryptionCapacity() & IPMSG_RSA_2048 ) {
		pubKeyMethod = IPMSG_RSA_2048;
	}
#endif	//SUPPORT_RSA_2048
#endif	//WINCOMPAT

#ifdef SUPPORT_RSA_1024
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_1024 && host.EncryptionCapacity() & IPMSG_RSA_1024 ) {
		pubKeyMethod = IPMSG_RSA_1024;
	}
#endif	//SUPPORT_RSA_1024

#ifdef SUPPORT_RSA_512
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_512  && host.EncryptionCapacity() & IPMSG_RSA_512 ) {
		pubKeyMethod = IPMSG_RSA_512;
	}
#endif	//SUPPORT_RSA_512
	//�Ź沽����ʤ��Τǡ�ʿʸ��������
	if ( pubKeyMethod == 0UL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::EncryptMsg encryptionCapacity(%lx)\n", encryptionCapacity );fflush(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg host.EncryptionCapacity()(%lx)\n", host.EncryptionCapacity() );fflush(stdout);
		printf("IpMessengerAgentImpl::EncryptMsg pubKeyMethod == 0UL\n");fflush(stdout);
#endif
		IPMSG_FUNC_RETURN( false );
	}

	RSA *rsa = RSA_new();
	rsa->e = BN_new();
	if ( BN_hex2bn( &rsa->e, host.EncryptMethodHex().c_str() ) == 0 ){
#if defined(INFO) || !defined(NDEBUG)
		char errbuf[ERR_BUF_SIZE];
		printf( "IpMessengerAgentImpl::EncryptMsg BN_bn2hex err=%s\n", ERR_error_string(ERR_get_error(), errbuf));fflush(stdout);
#endif
		RSA_free( rsa );
		IPMSG_FUNC_RETURN( false );
	}
	rsa->n = BN_new();
	if ( BN_hex2bn( &rsa->n, host.PubKeyHex().c_str() ) == 0 ){
#if defined(INFO) || !defined(NDEBUG)
		char errbuf[ERR_BUF_SIZE];
		printf( "IpMessengerAgentImpl::EncryptMsg BN_bn2hex err=%s\n", ERR_error_string(ERR_get_error(), errbuf));fflush(stdout);
#endif
		RSA_free( rsa );
		IPMSG_FUNC_RETURN( false );
	}

	memset( iv, 0, sizeof( iv ) );
	
	unsigned char sharekey[EVP_MAX_KEY_LENGTH];
	int key_bytes_size = 0;
	unsigned long shareKeyMethod = 0UL;
#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_BLOWFISH_256 && host.EncryptionCapacity() & IPMSG_BLOWFISH_256 ) {
		shareKeyMethod = IPMSG_BLOWFISH_256;
		key_bytes_size = 256/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT

#ifdef SUPPORT_BLOWFISH_128
#ifdef WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_BLOWFISH_128 && host.EncryptionCapacity() & IPMSG_BLOWFISH_128 && pubKeyMethod == IPMSG_RSA_1024 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_BLOWFISH_128 && host.EncryptionCapacity() & IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		shareKeyMethod = IPMSG_BLOWFISH_128;
		key_bytes_size = 128/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_256      && host.EncryptionCapacity() & IPMSG_RC2_256 ) {
		shareKeyMethod = IPMSG_RC2_256;
		key_bytes_size = 256/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_128      && host.EncryptionCapacity() & IPMSG_RC2_128 ) {
		shareKeyMethod = IPMSG_RC2_128;
		key_bytes_size = 128/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifdef SUPPORT_RC2_40
#ifdef WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_40       && host.EncryptionCapacity() & IPMSG_RC2_40 && pubKeyMethod == IPMSG_RSA_512 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && encryptionCapacity & IPMSG_RC2_40       && host.EncryptionCapacity() & IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		shareKeyMethod = IPMSG_RC2_40;
		key_bytes_size = 40/8;
		RAND_bytes( sharekey, key_bytes_size );
	}
#endif	//SUPPORT_RC2_40
	//�Ź沽����ʤ��Τǡ�ʿʸ��������
	if ( shareKeyMethod == 0UL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::EncryptMsg shareKeyMethod == 0UL\n");fflush(stdout);
#endif
		RSA_free( rsa );
		IPMSG_FUNC_RETURN( false );
	}
	int enc_key_size = RSA_size( rsa );
	unsigned char *enc_key = (unsigned char *)calloc( enc_key_size + 1, 1 );
#if defined(INFO) || !defined(NDEBUG)
	printf( "IpMessengerAgentImpl::EncryptMsg enc_key_size(%d)\n", enc_key_size );fflush(stdout);
#endif
	if ( enc_key == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::EncryptMsg enc_key == NULL\n");fflush(stdout);
#endif
		RSA_free( rsa );
		IPMSG_FUNC_RETURN( false );
	}
	//���̸���RSA�������ǰŹ沽��
	int enc_key_len = RSA_public_encrypt( key_bytes_size, sharekey, enc_key, rsa, RSA_PKCS1_PADDING );
	if ( enc_key_len < 0 ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::EncryptMsg enc_key_len < 0\n");fflush(stdout);
#endif
		RSA_free( rsa );
		free( enc_key );
		IPMSG_FUNC_RETURN( false );
	}
	//���̸�����ʸ��Ź沽��
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init( &ctx );
	int seal_init_ret = 0;
#ifdef SUPPORT_RC2_40
#ifdef WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 && pubKeyMethod == IPMSG_RSA_512 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_rc2_40_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//��Ĺ������
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == IPMSG_RC2_128 ) {
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_rc2_64_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//��Ĺ������
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if( shareKeyMethod == IPMSG_RC2_256 ) {
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_rc2_64_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//��Ĺ������
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifdef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_128
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 && pubKeyMethod == IPMSG_RSA_1024 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_bf_cbc(), NULL, NULL );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//��Ĺ������
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == IPMSG_BLOWFISH_256 ) {
		seal_init_ret = EVP_EncryptInit( &ctx, EVP_bf_cbc(), (unsigned char*)sharekey, iv );
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//��Ĺ������
		seal_init_ret = EVP_EncryptInit( &ctx, NULL, (unsigned char*)sharekey, NULL );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	char *enc_buf = (char *)calloc( optBufLen + key_bytes_size + 1, 1 );
	if ( enc_buf == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::EncryptMsg enc_buf == NULL\n");fflush(stdout);
#endif
		RSA_free( rsa );
		free( enc_key );
		IPMSG_FUNC_RETURN( false );
	}
	int ol;
	int o_len = 0;
	int ret;
	// �Хåե��������ޤǷ����֤���
	for( int i = 0; i < optBufLen / key_bytes_size; i++ ){
		ret = EVP_EncryptUpdate( &ctx, (unsigned char*)&enc_buf[o_len], &ol, &optBuf[o_len], key_bytes_size );
		o_len += ol;
	}
	if( optBufLen % key_bytes_size != 0 ){
		ret = EVP_EncryptUpdate( &ctx, (unsigned char*)&enc_buf[o_len], &ol, &optBuf[o_len], optBufLen % key_bytes_size );
		o_len += ol;
	}
	ret = EVP_EncryptFinal( &ctx, (unsigned char*)&enc_buf[o_len], &ol );
	o_len += ol;

	int ob_len = 8 + 1 + ( enc_key_len * 2 ) + 1 + ( o_len * 2 ) + 1;
	char *out_buf = (char *)calloc( ob_len + 1, 1 );
	if ( out_buf == NULL ){
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::EncryptMsg out_buf == NULL\n");fflush(stdout);
#endif
		RSA_free( rsa );
		free( enc_key );
		free( enc_buf );
		IPMSG_FUNC_RETURN( false );
	}
	snprintf( (char *)out_buf, ob_len, "%lx:", pubKeyMethod | shareKeyMethod );
	for( int i = 0; i < enc_key_len; i++ ) {
		char pout_hex[3];
		IpMsgUCharToHexString( pout_hex, (unsigned char)enc_key[i] );
		strcat( (char *)out_buf, pout_hex );
	}
	strcat( (char *)out_buf, PACKET_DELIMITER_STRING );
	for( int i = 0; i < o_len; i++ ) {
		char pout_hex[3];
		IpMsgUCharToHexString( pout_hex, (unsigned char)enc_buf[i] );
		strcat( (char *)out_buf, pout_hex );
	}
	*enc_optBufLen = strlen( (char *)out_buf );
	if ( opt_size > *enc_optBufLen ) {
		memset( optBuf, 0, *enc_optBufLen + 1 );
		memcpy( optBuf, out_buf, *enc_optBufLen );
	}
	RSA_free( rsa );
	free( enc_key );
	free( enc_buf );
	free( out_buf );
	if ( opt_size > *enc_optBufLen ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::EncryptMsg Return value is true.\n");fflush(stdout);
#endif
		IPMSG_FUNC_RETURN( true );
	}

#if defined(INFO) || !defined(NDEBUG)
	printf("IpMessengerAgentImpl::EncryptMsg Return value is false.\n");fflush(stdout);
#endif
	IPMSG_FUNC_RETURN( false );
#else	//HAVE_OPENSSL
	IPMSG_FUNC_RETURN( false );
#endif	//HAVE_OPENSSL
}

/**
 * ��å��������沽��
 * @param packet �ѥ��åȥ��֥������ȡʻ��ȡ�
 * @retval true:���沽����
 * @retval false:���沽����
 */
bool
IpMessengerAgentImpl::DecryptMsg( const Packet &packet, std::string& msg )
{
	IPMSG_FUNC_ENTER( "bool IpMessengerAgentImpl::DecryptMsg( const Packet &packet, std::string& msg )" );
#ifdef HAVE_OPENSSL
	EVP_CIPHER_CTX ctx;
	unsigned char iv[EVP_MAX_IV_LENGTH];

	char *buf = (char *)calloc( packet.Option().size() + 1, 1);
	if ( buf == NULL ){
		IPMSG_FUNC_RETURN( false );
	}
	memcpy( buf, packet.Option().c_str(), packet.Option().size());
	char *file_ptr = &buf[strlen( buf ) + 1];
	char *file_info = (char *)calloc( packet.Option().size(), 1 );
	int file_info_len = strlen( file_ptr );
	if ( file_info == NULL ) {
		free( buf );
		IPMSG_FUNC_RETURN( false );
	}
	memcpy( file_info, file_ptr, file_info_len );
	IpMsgPrintBuf("file_ptr:", file_ptr, file_info_len);
	IpMsgPrintBuf("file_info:", file_info, file_info_len);

	char *token = buf;
	char *nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		IPMSG_FUNC_RETURN( false );
	}
	char *dmyptr;
	unsigned long methods = strtoul( token, &dmyptr, 16 );

	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		IPMSG_FUNC_RETURN( false );
	}
	std::string ekey = token;

	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token == NULL ) {
		free( buf );
		free( file_info );
		IPMSG_FUNC_RETURN( false );
	}
	std::string emsg = token;

	std::string esign = "";
	token = nextpos;
	token = strtok_r( token, PACKET_DELIMITER_STRING, &nextpos );
	if ( token != NULL ) {
		esign = token;
	}
	free( buf );
	RSA *rsa = NULL;
	int rsa_bits = 0;
	unsigned long pubKeyMethod = 0UL;
#ifndef WINCOMPAT
#ifdef SUPPORT_RSA_2048
	if ( pubKeyMethod == 0UL && methods & IPMSG_RSA_2048 ) {
		pubKeyMethod = IPMSG_RSA_2048;
		rsa_bits = 2048/8;
		rsa = RsaMax;
	}
#endif	//SUPPORT_RSA_2048
#endif	//WINCOMPAT

#ifdef SUPPORT_RSA_1024
	if ( pubKeyMethod == 0UL && methods & IPMSG_RSA_1024 ) {
		pubKeyMethod = IPMSG_RSA_1024;
		rsa_bits = 1024/8;
		rsa = RsaMid;
	}
#endif	//SUPPORT_RSA_1024

#ifdef SUPPORT_RSA_512
	if ( pubKeyMethod == 0UL && methods & IPMSG_RSA_512 ) {
		pubKeyMethod = IPMSG_RSA_512;
		rsa_bits = 512/8;
		rsa = RsaMin;
	}
#endif	//SUPPORT_RSA_512
	//�Ź沽����Ƥ��ʤ���
	if ( pubKeyMethod == 0UL ) {
		IPMSG_FUNC_RETURN( false );
	}
	//�ѥǥ��󥰤�ޤॵ����
	int ekey_len = ekey.length() / 2;
	if ( ekey_len % rsa_bits > 0 ) {
		ekey_len = ( ( ekey.length() / 2 ) / rsa_bits ) * ( rsa_bits + 1 );
	}
	unsigned char *ek = (unsigned char *)calloc( ekey_len + 1, 1 );
	if ( ek == NULL ) {
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::DecryptMsg calloc 1\n");fflush(stdout);
#endif
		perror("calloc");
		IPMSG_FUNC_RETURN( false );
	}
	unsigned char *ekp = ek;
	for( unsigned int i = 0; i < ekey.length(); i += 2 ) {
		unsigned char ekc [3];
		ekc[0] = ekey.at( i );
		ekc[1] = ekey.at( i + 1 );
		ekc[2] = '\0';
		*ekp = (unsigned char)strtoul( (char *)ekc, &dmyptr, 16 );
		ekp++;
	}
	int ekl = ekey_len;
	unsigned long shareKeyMethod = 0UL;
	int key_bytes_size = 0;
#ifdef SUPPORT_RC2_40
#ifdef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_512 && shareKeyMethod == 0UL && methods & IPMSG_RC2_40 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		key_bytes_size = 40/8;
		shareKeyMethod = IPMSG_RC2_40;
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::DecryptMsg IPMSG_RC2_40\n");fflush(stdout);
#endif
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_128 ) {
		key_bytes_size = 128/8;
		shareKeyMethod = IPMSG_RC2_128;
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::DecryptMsg IPMSG_RC2_128\n");fflush(stdout);
#endif
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if ( shareKeyMethod == 0UL && methods & IPMSG_RC2_256 ) {
		key_bytes_size = 256/8;
		shareKeyMethod = IPMSG_RC2_256;
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::DecryptMsg IPMSG_RC2_256\n");fflush(stdout);
#endif
	}
#endif	//SUPPORT_RC2_256

#endif	//WINCOMPAT
#ifdef SUPPORT_BLOWFISH_128
#ifdef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_1024 && shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_128 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		key_bytes_size = 128/8;
		shareKeyMethod = IPMSG_BLOWFISH_128;
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::DecryptMsg IPMSG_BF_128\n");fflush(stdout);
#endif
	}
#endif	//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == 0UL && methods & IPMSG_BLOWFISH_256 ){
		key_bytes_size = 256/8;
		shareKeyMethod = IPMSG_BLOWFISH_256;
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::DecryptMsg IPMSG_BF_256\n");fflush(stdout);
#endif
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	//�Ź沽����Ƥ��ʤ���
	if ( shareKeyMethod == 0UL ) {
		free( file_info );
		free( ek );
		IPMSG_FUNC_RETURN( false );
	}
	unsigned char *emsg_buf = (unsigned char *)calloc( emsg.length() + 1, 1 );
	if ( emsg_buf == NULL ) {
#if defined(INFO) || !defined(NDEBUG)
		fprintf( stderr, "IpMessengerAgentImpl::DecryptMsg calloc 2\n" );fflush(stdout);
#endif
		perror("calloc");
		free( file_info );
		free( ek );
		IPMSG_FUNC_RETURN( false );
	}
	int data_len = 0;
	for( unsigned int i = 0; i < emsg.length(); i += 2 ) {
		unsigned char emc [3];
		emc[0] = emsg.at( i );
		emc[1] = emsg.at( i + 1 );
		emc[2] = '\0';
#if defined(INFO) || !defined(NDEBUG)
		printf("IpMessengerAgentImpl::DecryptMsg %d:emc=[%s]", data_len, emc);fflush(stdout);
#endif
		emsg_buf[data_len] = (unsigned char)strtoul( (char *)emc, &dmyptr, 16 );
#if defined(INFO) || !defined(NDEBUG)
		printf("[%02x]\n", emsg_buf[data_len]);fflush(stdout);
#endif
		data_len++;
	}

	EVP_PKEY pubkey;
	EVP_PKEY_set1_RSA( &pubkey, rsa );
	int open_init_ret = 0;
	memset( iv, 0, sizeof( iv ) );
#ifdef SUPPORT_RC2_40
#ifndef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_512 && shareKeyMethod == IPMSG_RC2_40 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_RC2_40 ) {
#endif	//WINCOMPAT
		open_init_ret = EVP_OpenInit( &ctx, EVP_rc2_40_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			IPMSG_FUNC_RETURN( false );
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//��Ĺ������
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_RC2_40

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_128
	if ( shareKeyMethod == IPMSG_RC2_128 ) {
		open_init_ret = EVP_OpenInit( &ctx, EVP_rc2_64_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			IPMSG_FUNC_RETURN( false );
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//��Ĺ������
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_RC2_128
#endif	//WINCOMPAT

#ifndef WINCOMPAT
#ifdef SUPPORT_RC2_256
	if( shareKeyMethod == IPMSG_RC2_256 ) {
		open_init_ret = EVP_OpenInit( &ctx, EVP_rc2_64_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			IPMSG_FUNC_RETURN( false );
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//��Ĺ������
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_RC2_256
#endif	//WINCOMPAT

#ifdef SUPPORT_BLOWFISH_128
#ifdef WINCOMPAT
	if ( pubKeyMethod == IPMSG_RSA_1024 && shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#else	//WINCOMPAT
	if ( shareKeyMethod == IPMSG_BLOWFISH_128 ) {
#endif	//WINCOMPAT
		open_init_ret = EVP_OpenInit( &ctx, EVP_bf_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			IPMSG_FUNC_RETURN( false );
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//��Ĺ������
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif//SUPPORT_BLOWFISH_128

#ifndef WINCOMPAT
#ifdef SUPPORT_BLOWFISH_256
	if ( shareKeyMethod == IPMSG_BROWFISH_256 ) {
		open_init_ret = EVP_OpenInit( &ctx, EVP_bf_cbc(), ek, ekl, iv, &pubkey );
		if ( open_init_ret <= 0 ){
			free( file_info );
			free( ek );
			free( emsg_buf );
			IPMSG_FUNC_RETURN( false );
		}
		EVP_CIPHER_CTX_set_key_length( &ctx, key_bytes_size );				//��Ĺ������
		open_init_ret = EVP_OpenInit( &ctx, NULL, ek, ekl, iv, &pubkey );
	}
#endif	//SUPPORT_BLOWFISH_256
#endif	//WINCOMPAT
	if ( open_init_ret <= 0 ){
		free( file_info );
		free( ek );
		free( emsg_buf );
		IPMSG_FUNC_RETURN( false );
	}
	int tmp_len = 0;
	int tmp;
	unsigned char *optBuf = (unsigned char *)calloc( data_len + key_bytes_size + 1 + file_info_len + 1, 1 );
	if ( optBuf == NULL ){
		perror("calloc");
		free( file_info );
		free( ek );
		free( emsg_buf );
		IPMSG_FUNC_RETURN( false );
	}

	int ret;
	ret = EVP_OpenUpdate( &ctx, &optBuf[tmp_len], &tmp, &emsg_buf[tmp_len], data_len );
	tmp_len += tmp;
	ret = EVP_OpenFinal( &ctx, &optBuf[tmp_len], &tmp );
	tmp_len += tmp;
	free( ek );
	free( emsg_buf );
	optBuf[tmp_len] = '\0';
	
	if ( file_info_len > 0 ){
		IpMsgPrintBuf( "optBuf(1):", (char *)optBuf, tmp_len );
		IpMsgPrintBuf( "file_info:", (char *)file_info, file_info_len );
		memcpy( &optBuf[tmp_len+1], file_info, file_info_len );
		tmp_len += ( file_info_len + 1 );
	}
	msg = std::string( (char *)optBuf, tmp_len );
	IpMsgPrintBuf( "optBuf(2):", (char *)optBuf, tmp_len );
	free( optBuf );
	free( file_info );
	IPMSG_FUNC_RETURN( true );
#else	//HAVE_OPENSSL
	IPMSG_FUNC_RETURN( false );
#endif	//HAVE_OPENSSL
}

/**
 * ��Ŭ��RSA���֥������Ȥ����򤷤��ֵѤ��롣
 * @param cap ��ʬ�ΰŹ沽ǽ�Ϥ򼨤��ե饰��
 * @retval �Ź沽�˻��Ѥ���RSA���֥������ȡ�
 */
RSA *
IpMessengerAgentImpl::GetOptimizedRsa( unsigned long cap )
{
	IPMSG_FUNC_ENTER( "RSA *IpMessengerAgentImpl::GetOptimizedRsa( unsigned long cap )" );
	RSA *rsa = NULL;
	unsigned long pubKeyMethod = 0UL;
#ifdef SUPPORT_RSA_2048
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_2048 && cap& IPMSG_RSA_2048 ) {
		pubKeyMethod |= IPMSG_RSA_2048;
		rsa = RsaMax != NULL ? RsaMax : NULL;
	}
#endif	// SUPPORT_RSA_2048
#ifdef SUPPORT_RSA_1024
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_1024 && cap & IPMSG_RSA_1024 ) {
		pubKeyMethod |= IPMSG_RSA_1024;
		rsa = RsaMid != NULL ? RsaMid : NULL;
	}
#endif	// SUPPORT_RSA_1024
#ifdef SUPPORT_RSA_512
	if ( pubKeyMethod == 0UL && encryptionCapacity & IPMSG_RSA_512  && cap & IPMSG_RSA_512 ) {
		pubKeyMethod |= IPMSG_RSA_512;
		rsa = RsaMin != NULL ? RsaMin : NULL;
	}
#endif	// SUPPORT_RSA_512
	IPMSG_FUNC_RETURN( rsa );
}