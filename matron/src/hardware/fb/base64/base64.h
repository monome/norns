/*
	base64.c - by Joe DF (joedf@ahkscript.org)
	Released under the MIT License
	
	Revision: 2015-06-12 01:26:51
	
	Thank you for inspiration:
	http://www.codeproject.com/Tips/813146/Fast-base-functions-for-encode-decode
*/

#include <stdio.h>

//Base64 char table function - used internally for decoding
unsigned int b64_int(unsigned int ch);

// in_size : the number bytes to be encoded.
// Returns the recommended memory size to be allocated for the output buffer excluding the null byte
unsigned int b64e_size(unsigned int in_size);

// in_size : the number bytes to be decoded.
// Returns the recommended memory size to be allocated for the output buffer
unsigned int b64d_size(unsigned int in_size);

// in : buffer of "raw" binary to be encoded.
// in_len : number of bytes to be encoded.
// out : pointer to buffer with enough memory, user is responsible for memory allocation, receives null-terminated string
// returns size of output including null byte
unsigned int b64_encode(const unsigned char* in, unsigned int in_len, unsigned char* out);

// in : buffer of base64 string to be decoded.
// in_len : number of bytes to be decoded.
// out : pointer to buffer with enough memory, user is responsible for memory allocation, receives "raw" binary
// returns size of output excluding null byte
unsigned int b64_decode(const unsigned char* in, unsigned int in_len, unsigned char* out);

// file-version b64_encode
// Input : filenames
// returns size of output
unsigned int b64_encodef(char *InFile, char *OutFile);

// file-version b64_decode
// Input : filenames
// returns size of output
unsigned int b64_decodef(char *InFile, char *OutFile);