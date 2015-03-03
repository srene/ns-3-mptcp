/*
 * sha-digest.cc
 *
 *  Created on: Feb 8, 2013
 *      Author: sergi
 */

#include "sha-digest.h"

ShaDigest::ShaDigest() {
	// TODO Auto-generated constructor stub
	CryptoPP::SHA1 sha1;
	std::string source = "Hello";  //This will be randomly generated somehow
	std::string hash = "";
	CryptoPP::StringSource(source, true, new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash))));

}

ShaDigest::~ShaDigest() {
	// TODO Auto-generated destructor stub
}

