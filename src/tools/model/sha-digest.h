/*
 * sha-digest.h
 *
 *  Created on: Feb 8, 2013
 *      Author: sergi
 */
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

#ifndef SHA_DIGEST_H_
#define SHA_DIGEST_H_

class ShaDigest {
public:
	ShaDigest();
	virtual ~ShaDigest();
};

#endif /* SHA_DIGEST_H_ */
