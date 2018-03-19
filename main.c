/*
 * Copyright (C) 2012-2014 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _SVID_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <makestuff.h>
#include <libfpgalink.h>
#include <libbuffer.h>
#include <liberror.h>
#include <libdump.h>
#include <argtable2.h>
#include <readline/readline.h>
#include <readline/history.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif


// ---------------------------------------------------------------------------------------------------------------------------------------------------------

char key[32] = "00000000000000000000000000000001";

struct data{
    int coord[2];
    char* value[8];
};

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the null-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
void substring(char s[], char sub[], int p, int l) {
   int c = 0;

   while (c < l) {
      sub[c] = s[p+c-1];
      c++;
   }
   sub[c] = '\0';
}

int convertDecimalToBinary(int n)
{
    long long binaryNumber = 0;
    int remainder, i = 1;

    while (n!=0)
    {
        remainder = n%2;
        n /= 2;
        binaryNumber += remainder*i;
        i *= 10;
    }
    return binaryNumber;
}

int power(int a, int b){
    int res=1;
    for(int i=0; i<b;i++){
        res *=a;
    }
    return res;
}

int convertBinaryToDecimal(char *a){
    int res=0;
    for(int i=3; i>=0; --i){
        res+=(a[(3-i)]-48)*power(2,i);
    }
    return res;
}

char * binaryToHex(const char *inStr) {
    char hex[] = "0123456789abcdef";
    char *outStr;
    outStr=(char*)malloc(16);
    int len = strlen(inStr) / 4;
    int i = strlen(inStr) % 4;
    char current = 0;
    int l=0;
    while(len>0) {
        current = 0;
        for(i = 0; i < 4; ++i) {
            current = (current << 1) + (*inStr - '0');
            inStr++;
        }
        outStr[l] = hex[current-48];
        len-=1;
        l+=1;
    }
    return (char *)outStr;
}

char * binaryToHex32(const char * inStr){
  char hex[] = "0123456789abcdef";
  char *outStr;
  outStr=(char*)malloc(9);
  int len = strlen(inStr) / 4;
  int i = strlen(inStr) % 4;
  char current = 0;
  int l=0;
  while(len>0) {
      current = 0;
      for(i = 0; i < 4; ++i) {
          current = (current << 1) + (*inStr - '0');
          inStr++;
      }
      outStr[l] = hex[current];
      len-=1;
      l+=1;
  }
  *(outStr+8)='\0';

  return (char *)outStr;
}

char * bit_append(int b){
     char* c;
     c=(char *)malloc(4);
     int d;
     d=convertDecimalToBinary(b);
    if((d/100)==0){
        c[0]='0';
    }
    else{
        c[0]='1';
    }
    d=(d%100);
    if((d/10)==0){
        c[1]='0';
    }
    else{
        c[1]='1';
    }
    d=(d%10);
    if(d==0){
        c[2]='0';
    }
    else{
        c[2]='1';
    }
    c[3]='\0';
    return(char *)c;
 }

 bool lookup(int a, int b, struct data bla[100]){
     int i=0;
     bool res=false;
     while(i<100){
         if((bla[i].coord[0]==a)&&(bla[i].coord[1]==b)){
             res=true;
             break;
         }
         else{
             i+=1;
         }
     }

     return res;
 }

 int find(int a, int b, struct data bla[100]){
     int i=0;
     int res=0;
     while(i<100){
         if((bla[i].coord[0]==a)&&(bla[i].coord[1]==b)){
             res=i;
             break;
         }
         else{
             i+=1;
         }
     }

     return res;
 }


const char* getfield(char* line, int num)
{
    const char* tok;
    for (tok = strtok(line, ";");
            tok && *tok;
            tok = strtok(NULL, ";\n"))
    {
        if (!--num)
            return tok;
    }
    return NULL;
}

int countOnes(char * s){
    int count = 0;
    for(int i= 0;i<32;i+=1){
        if(s[i]=='1')
            count+=1;
    }
    return count;
}

char xor(char x, char y){
    if(x==y)
        return '0';
    else
        return '1';
}

char * xor32(char* a, char* b){
    char *c;
    c=(char *)malloc(32);
    for(int i=0; i<8; i++){
        c[4*i]=xor(a[4*i], b[0]);
        c[4*i+1]=xor(a[4*i+1],b[1]);
        c[4*i+2]=xor(a[4*i+2],b[2]);
        c[4*i+3]=xor(a[4*i+3],b[3]);
    }
    return (char *)c;
}

char * add1(char * b){
    char * t;
    t=(char *) malloc(4);
    t=b;
    if(t[3]=='0'){
        t[3]='1';
    }
    else{
        t[3]='0';
        if(t[2]=='0')
            t[2]='1';
        else{
            t[2]='0';
            if(t[1]=='0')
                t[1]='1';
            else{
                t[1]='0';
                if(t[0]=='0')
                    t[0]='1';
                else
                    t[0]='0';
            }
        }
    }

    return (char *)t;
}

char * sub1(char * b){
    char * t;
    t=(char *) malloc(4);
    t=b;
    if(t[3]=='1'){
        t[3]='0';
    }
    else{
        t[3]='1';
        if(t[2]=='1')
            t[2]='0';
        else{
            t[2]='1';
            if(t[1]=='1')
                t[1]='0';
            else{
                t[1]='1';
                if(t[0]=='1')
                    t[0]='0';
                else
                    t[0]='1';
            }
        }
    }

    return (char *)t;
}

char* decrypt(char * cypher_text){
    int n = 32 - countOnes(key);
    char *c;
    c = (char *)malloc(33);
    c = cypher_text;
    char t0,t1,t2,t3;
    t3 = xor(xor(xor(xor(xor(xor(xor(key[7],key[3]),key[11]),key[15]),key[19]),key[23]),key[27]),key[31]);
    t2 = xor(xor(xor(xor(xor(xor(xor(key[6],key[2]),key[10]),key[14]),key[18]),key[22]),key[26]),key[30]);
    t1 = xor(xor(xor(xor(xor(xor(xor(key[5],key[1]),key[9]),key[13]),key[17]),key[21]),key[25]),key[29]);
    t0 = xor(xor(xor(xor(xor(xor(xor(key[4],key[0]),key[8]),key[12]),key[16]),key[20]),key[24]),key[28]);
    char t[5];
    t[3]=t3;
    t[2]=t2;
    t[1]=t1;
    t[0]=t0;
		t[4]='\0';
    sub1(t);
    for(int i =0;i<n;i+=1){
        c = xor32(c,t);
    sub1(t);
    }
		c[32]='\0';
    return(char *)c;
}
char* encrypt(char * plain_text){
    int n = countOnes(key);
    char *c;
    c = (char *)malloc(32);
    c = plain_text;
    char t0,t1,t2,t3;
    t3 = xor(xor(xor(xor(xor(xor(xor(key[7],key[3]),key[11]),key[15]),key[19]),key[23]),key[27]),key[31]);
    t2 = xor(xor(xor(xor(xor(xor(xor(key[6],key[2]),key[10]),key[14]),key[18]),key[22]),key[26]),key[30]);
    t1 = xor(xor(xor(xor(xor(xor(xor(key[5],key[1]),key[9]),key[13]),key[17]),key[21]),key[25]),key[29]);
    t0 = xor(xor(xor(xor(xor(xor(xor(key[4],key[0]),key[8]),key[12]),key[16]),key[20]),key[24]),key[28]);
    char t[5];
    t[3]=t3;
    t[2]=t2;
    t[1]=t1;
    t[0]=t0;
		t[4]='\0';
    for(int i =0;i<n;i+=1){
        c = xor32(c,t);
        add1(t);
    }
    char *ec = (char *)malloc(8);
    ec = binaryToHex32(c);
    return(char *)ec;
}


int getx(char *a){
    char *b=(char *)malloc(4);
    b[0]=a[24];
    b[1]=a[25];
    b[2]=a[26];
    b[3]=a[27];
    int res;
    res=convertBinaryToDecimal(b);
    return res;
}

int gety(char *a){
    char *b=(char *)malloc(4);
    b[0]=a[28];
    b[1]=a[29];
    b[2]=a[30];
    b[3]=a[31];
    int res;
    res=convertBinaryToDecimal(b);
    return res;
}

char * get_answer(char *coordinates){
    FILE* stream;
    stream = fopen("/home/vikrant/Desktop/track_data.csv", "r");
    char line[1024];
    int mat[100][5];
    int t=0;
    while (fgets(line, 1024, stream))
    {
        char* tmp = strdup(line);
        mat[t][0]=tmp[0]-48;
        mat[t][1]=tmp[2]-48;
        mat[t][2]=tmp[4]-48;
        mat[t][3]=tmp[6]-48;
        mat[t][4]=tmp[8]-48;
        t++;
        free(tmp);
    }
    struct data str[100];
    for(int i=0; i<100; i++){
        str[i].value[0]="00000000";
        str[i].value[1]="00001000";
        str[i].value[2]="00010000";
        str[i].value[3]="00011000";
        str[i].value[4]="00100000";
        str[i].value[5]="00101000";
        str[i].value[6]="00110000";
        str[i].value[7]="00111000";
    }
    int j=0;
    for(int i=0; i<t; i+=1){
      if(lookup(mat[i][0], mat[i][1], str)){
          int index = find(mat[i][0], mat[i][1], str);
          char *new_str=malloc(10);
					char *y;
					if(mat[i][3]==1){
						y="1";
					}
					else{
						y="0";
					}
          sprintf(new_str,"%s%s%s%s","1",y,bit_append(mat[i][2]),bit_append(mat[i][4]));
          str[index].value[mat[i][2]]=new_str;
      }
      else{
          str[j].coord[0]=mat[i][0];
          str[j].coord[1]=mat[i][1];
          char *new_str=malloc(10);
        	char* y;
					if(mat[i][3]==1){
						y="1";
					}
					else{
						y="0";
					}
          sprintf(new_str,"%s%s%s%s","1",y,bit_append(mat[i][2]),bit_append(mat[i][4]));
          str[j].value[mat[i][2]]=new_str;
          j++;
      }
    }



    int x =getx(coordinates);
    int y =gety(coordinates);
    int b=find(x,y,str);
    char * plain_text=(char* )malloc(65);
    for(int i=0; i<8; i++){
        for(int j=0; j<8; j++){
            plain_text[8*(7-i)+j]=*str[b].value[i];
            str[b].value[i]++;
        }
    }
    plain_text[64]='\0';
		char *first=(char *)malloc(33);
    char *second=(char *)malloc(33);
    for(int i=0; i<32; i++){
        first[i]=plain_text[i];
        second[i]=plain_text[32+i];
    }
    first[32]='\0';
    second[32]='\0';
    char *first_e=(char *)malloc(32);
    char *second_e=(char *)malloc(32);
    first_e=encrypt(first);
    second_e=encrypt(second);
    char * encrypted_text=(char *)malloc(64);
    sprintf(encrypted_text,"%s%s",first_e,second_e);
    char*res=(char *)malloc(16);
    res=encrypted_text;
    return res ;
 }

 char *substring_1(char *string, int position, int length)
 {
    char *pointer;
    int c;

    pointer = malloc(length+1);

    if (pointer == NULL)
    {
       printf("Unable to allocate memory.\n");
       exit(1);
    }

    for (c = 0 ; c < length ; c++)
    {
       *(pointer+c) = *(string+position-1);
       string++;
    }

    *(pointer+c) = '\0';

    return pointer;
 }
// ---------------------------------------------------------------------------------------------------------------------------------------------------------

char f2hData[33];

char *conv_int_eight(int number){
     char *res1=(char *)malloc(9);
     int i,d,count;
     count =0;
    for(i = 7; i >= 0; i--) {
        d=number >>i;
        if(d & 1){
           *(res1+count)=1+'0';
        }
        else{
            *(res1+count)=0+'0';
        }
        count++;
    }
    *(res1+count)='\0';
    return res1;

}

void getdata(const uint8 *a){
  int b=(unsigned int)*a;
	a++;
  int b1=(unsigned int)*a;
	a++;
  int b2=(unsigned int)*a;
	a++;
  int b3=(unsigned int)*a;
	char *newstr=malloc(33);
  sprintf(newstr,"%s%s%s%s",conv_int_eight(b),conv_int_eight(b1),conv_int_eight(b2),conv_int_eight(b3));
    for(int i=0; i< 32; ++i){
        f2hData[i]=newstr[i];
    }
		f2hData[32]='\0';
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------



bool sigIsRaised(void);
void sigRegisterHandler(void);

static const char *ptr;
static bool enableBenchmarking = false;

static bool isHexDigit(char ch) {
	return
		(ch >= '0' && ch <= '9') ||
		(ch >= 'a' && ch <= 'f') ||
		(ch >= 'A' && ch <= 'F');
}

static uint16 calcChecksum(const uint8 *data, size_t length) {
	uint16 cksum = 0x0000;
	while ( length-- ) {
		cksum = (uint16)(cksum + *data++);
	}
	return cksum;
}

static bool getHexNibble(char hexDigit, uint8 *nibble) {
	if ( hexDigit >= '0' && hexDigit <= '9' ) {
		*nibble = (uint8)(hexDigit - '0');
		return false;
	} else if ( hexDigit >= 'a' && hexDigit <= 'f' ) {
		*nibble = (uint8)(hexDigit - 'a' + 10);
		return false;
	} else if ( hexDigit >= 'A' && hexDigit <= 'F' ) {
		*nibble = (uint8)(hexDigit - 'A' + 10);
		return false;
	} else {
		return true;
	}
}

static int getHexByte(uint8 *byte) {
	uint8 upperNibble;
	uint8 lowerNibble;
	if ( !getHexNibble(ptr[0], &upperNibble) && !getHexNibble(ptr[1], &lowerNibble) ) {
		*byte = (uint8)((upperNibble << 4) | lowerNibble);
		byte += 2;
		return 0;
	} else {
		return 1;
	}
}

static const char *const errMessages[] = {
	NULL,
	NULL,
	"Unparseable hex number",
	"Channel out of range",
	"Conduit out of range",
	"Illegal character",
	"Unterminated string",
	"No memory",
	"Empty string",
	"Odd number of digits",
	"Cannot load file",
	"Cannot save file",
	"Bad arguments"
};

typedef enum {
	FLP_SUCCESS,
	FLP_LIBERR,
	FLP_BAD_HEX,
	FLP_CHAN_RANGE,
	FLP_CONDUIT_RANGE,
	FLP_ILL_CHAR,
	FLP_UNTERM_STRING,
	FLP_NO_MEMORY,
	FLP_EMPTY_STRING,
	FLP_ODD_DIGITS,
	FLP_CANNOT_LOAD,
	FLP_CANNOT_SAVE,
	FLP_ARGS
} ReturnCode;

static ReturnCode doRead(
	struct FLContext *handle, uint8 chan, uint32 length, FILE *destFile, uint16 *checksum,
	const char **error)
{
	ReturnCode retVal = FLP_SUCCESS;
	uint32 bytesWritten;
	FLStatus fStatus;
	uint32 chunkSize;
	const uint8 *recvData;
	uint32 actualLength;
	const uint8 *ptr;
	uint16 csVal = 0x0000;
	#define READ_MAX 65536

	// Read first chunk
	chunkSize = length >= READ_MAX ? READ_MAX : length;
	fStatus = flReadChannelAsyncSubmit(handle, chan, chunkSize, NULL, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
	length = length - chunkSize;

	while ( length ) {
		// Read chunk N
		chunkSize = length >= READ_MAX ? READ_MAX : length;
		fStatus = flReadChannelAsyncSubmit(handle, chan, chunkSize, NULL, error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
		length = length - chunkSize;

		// Await chunk N-1
		fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");

		// Write chunk N-1 to file
		bytesWritten = (uint32)fwrite(recvData, 1, actualLength, destFile);
		CHECK_STATUS(bytesWritten != actualLength, FLP_CANNOT_SAVE, cleanup, "doRead()");

		// Checksum chunk N-1
		chunkSize = actualLength;
		ptr = recvData;
		while ( chunkSize-- ) {
			csVal = (uint16)(csVal + *ptr++);
		}
	}

	// Await last chunk
	fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");

	// Write last chunk to file
	bytesWritten = (uint32)fwrite(recvData, 1, actualLength, destFile);
	CHECK_STATUS(bytesWritten != actualLength, FLP_CANNOT_SAVE, cleanup, "doRead()");

	getdata(recvData);
	// Checksum last chunk
	chunkSize = actualLength;
	ptr = recvData;
	while ( chunkSize-- ) {
		csVal = (uint16)(csVal + *ptr++);
	}

	// Return checksum to caller
	*checksum = csVal;
cleanup:
	return retVal;
}

static ReturnCode doWrite(
	struct FLContext *handle, uint8 chan, FILE *srcFile, size_t *length, uint16 *checksum,
	const char **error)
{
	ReturnCode retVal = FLP_SUCCESS;
	size_t bytesRead, i;
	FLStatus fStatus;
	const uint8 *ptr;
	uint16 csVal = 0x0000;
	size_t lenVal = 0;
	#define WRITE_MAX (65536 - 5)
	uint8 buffer[WRITE_MAX];

	do {
		// Read Nth chunk
		bytesRead = fread(buffer, 1, WRITE_MAX, srcFile);
		if ( bytesRead ) {
			// Update running total
			lenVal = lenVal + bytesRead;

			// Submit Nth chunk
			fStatus = flWriteChannelAsync(handle, chan, bytesRead, buffer, error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doWrite()");

			// Checksum Nth chunk
			i = bytesRead;
			ptr = buffer;
			while ( i-- ) {
				csVal = (uint16)(csVal + *ptr++);
			}
		}
	} while ( bytesRead == WRITE_MAX );

	// Wait for writes to be received. This is optional, but it's only fair if we're benchmarking to
	// actually wait for the work to be completed.
	fStatus = flAwaitAsyncWrites(handle, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doWrite()");

	// Return checksum & length to caller
	*checksum = csVal;
	*length = lenVal;
cleanup:
	return retVal;
}

static int parseLine(struct FLContext *handle, const char *line, const char **error) {
	ReturnCode retVal = FLP_SUCCESS, status;
	FLStatus fStatus;
	struct Buffer dataFromFPGA = {0,};
	BufferStatus bStatus;
	uint8 *data = NULL;
	char *fileName = NULL;
	FILE *file = NULL;
	double totalTime, speed;
	#ifdef WIN32
		LARGE_INTEGER tvStart, tvEnd, freq;
		DWORD_PTR mask = 1;
		SetThreadAffinityMask(GetCurrentThread(), mask);
		QueryPerformanceFrequency(&freq);
	#else
		struct timeval tvStart, tvEnd;
		long long startTime, endTime;
	#endif
	bStatus = bufInitialise(&dataFromFPGA, 1024, 0x00, error);
	CHECK_STATUS(bStatus, FLP_LIBERR, cleanup);
	ptr = line;
	do {
		while ( *ptr == ';' ) {
			ptr++;
		}
		switch ( *ptr ) {
		case 'r':{
			uint32 chan;
			uint32 length = 1;
			char *end;
			ptr++;

			// Get the channel to be read:
			errno = 0;
			chan = (uint32)strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
			ptr = end;

			// Only three valid chars at this point:
			CHECK_STATUS(*ptr != '\0' && *ptr != ';' && *ptr != ' ', FLP_ILL_CHAR, cleanup);

			if ( *ptr == ' ' ) {
				ptr++;

				// Get the read count:
				errno = 0;
				length = (uint32)strtoul(ptr, &end, 16);
				CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);
				ptr = end;

				// Only three valid chars at this point:
				CHECK_STATUS(*ptr != '\0' && *ptr != ';' && *ptr != ' ', FLP_ILL_CHAR, cleanup);
				if ( *ptr == ' ' ) {
					const char *p;
					const char quoteChar = *++ptr;
					CHECK_STATUS(
						(quoteChar != '"' && quoteChar != '\''),
						FLP_ILL_CHAR, cleanup);

					// Get the file to write bytes to:
					ptr++;
					p = ptr;
					while ( *p != quoteChar && *p != '\0' ) {
						p++;
					}
					CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
					fileName = malloc((size_t)(p - ptr + 1));
					CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
					CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
					strncpy(fileName, ptr, (size_t)(p - ptr));
					fileName[p - ptr] = '\0';
					ptr = p + 1;
				}
			}
			if ( fileName ) {
				uint16 checksum = 0x0000;

				// Open file for writing
				file = fopen(fileName, "wb");
				CHECK_STATUS(!file, FLP_CANNOT_SAVE, cleanup);
				free(fileName);
				fileName = NULL;

				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
          printf("%s\n", "calling doRead of WIN32");
          status = doRead(handle, (uint8)chan, length, file, &checksum, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
          printf("%s\n", "calling doRead");
					gettimeofday(&tvStart, NULL);
					status = doRead(handle, (uint8)chan, length, file, &checksum, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Read %d bytes (checksum 0x%04X) from channel %d at %f MiB/s\n",
						length, checksum, chan, speed);
				}
				CHECK_STATUS(status, status, cleanup);

				// Close the file
				fclose(file);
				file = NULL;
			} else {
				size_t oldLength = dataFromFPGA.length;
				bStatus = bufAppendConst(&dataFromFPGA, 0x00, length, error);
				CHECK_STATUS(bStatus, FLP_LIBERR, cleanup);
				#ifdef WIN32
          printf("%s\n","elseWin32" );
					QueryPerformanceCounter(&tvStart);
					fStatus = flReadChannel(handle, (uint8)chan, length, dataFromFPGA.data + oldLength, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
          printf("%s\n", "reading from fpga");
          fStatus = flReadChannel(handle, (uint8)chan, length, dataFromFPGA.data + oldLength, error);
          uint8* temp = dataFromFPGA.data + oldLength;
          getdata(temp);
          gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Read %d bytes (checksum 0x%04X) from channel %d at %f MiB/s\n",
						length, calcChecksum(dataFromFPGA.data + oldLength, length), chan, speed);
				}
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			}
			break;
		}
		case 'w':{
			unsigned long int chan;
			size_t length = 1, i;
			char *end, ch;
			const char *p;
			ptr++;
      printf("%s\n","writing" );
			// Get the channel to be written:
			errno = 0;
			chan = strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
			ptr = end;

			// There must be a space now:
			CHECK_STATUS(*ptr != ' ', FLP_ILL_CHAR, cleanup);

			// Now either a quote or a hex digit
		   ch = *++ptr;
			if ( ch == '"' || ch == '\'' ) {
				uint16 checksum = 0x0000;

				// Get the file to read bytes from:
				ptr++;
				p = ptr;
				while ( *p != ch && *p != '\0' ) {
					p++;
				}
				CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
				fileName = malloc((size_t)(p - ptr + 1));
				CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
				CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
				strncpy(fileName, ptr, (size_t)(p - ptr));
				fileName[p - ptr] = '\0';
				ptr = p + 1;  // skip over closing quote

				// Open file for reading
				file = fopen(fileName, "rb");
				CHECK_STATUS(!file, FLP_CANNOT_LOAD, cleanup);
				free(fileName);
				fileName = NULL;

				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
						length, checksum, chan, speed);
				}
				CHECK_STATUS(status, status, cleanup);

				// Close the file
				fclose(file);
				file = NULL;
			} else if ( isHexDigit(ch) ) {
				// Read a sequence of hex bytes to write
				uint8 *dataPtr;
				p = ptr + 1;
				while ( isHexDigit(*p) ) {
					p++;
				}
				CHECK_STATUS((p - ptr) & 1, FLP_ODD_DIGITS, cleanup);
				length = (size_t)(p - ptr) / 2;
				data = malloc(length);
				dataPtr = data;
				for ( i = 0; i < length; i++ ) {
					getHexByte(dataPtr++);
					ptr += 2;
				}
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
				if ( enableBenchmarking ) {
					printf(
						"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
						length, calcChecksum(data, length), chan, speed);
				}
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				free(data);
				data = NULL;
			} else {
				FAIL(FLP_ILL_CHAR, cleanup);
			}
			break;
		}
		case '+':{
			uint32 conduit;
			char *end;
			ptr++;

			// Get the conduit
			errno = 0;
			conduit = (uint32)strtoul(ptr, &end, 16);
			CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
			CHECK_STATUS(conduit > 255, FLP_CONDUIT_RANGE, cleanup);
			ptr = end;

			// Only two valid chars at this point:
			CHECK_STATUS(*ptr != '\0' && *ptr != ';', FLP_ILL_CHAR, cleanup);

			fStatus = flSelectConduit(handle, (uint8)conduit, error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			break;
		}
		default:
			FAIL(FLP_ILL_CHAR, cleanup);
		}
	} while ( *ptr == ';' );
	CHECK_STATUS(*ptr != '\0', FLP_ILL_CHAR, cleanup);

	dump(0x00000000, dataFromFPGA.data, dataFromFPGA.length);

cleanup:
	bufDestroy(&dataFromFPGA);
	if ( file ) {
		fclose(file);
	}
	free(fileName);
	free(data);
	if ( retVal > FLP_LIBERR ) {
		const int column = (int)(ptr - line);
		int i;
		fprintf(stderr, "%s at column %d\n  %s\n  ", errMessages[retVal], column, line);
		for ( i = 0; i < column; i++ ) {
			fprintf(stderr, " ");
		}
		fprintf(stderr, "^\n");
	}
	return retVal;
}

static const char *nibbles[] = {
	"0000",  // '0'
	"0001",  // '1'
	"0010",  // '2'
	"0011",  // '3'
	"0100",  // '4'
	"0101",  // '5'
	"0110",  // '6'
	"0111",  // '7'
	"1000",  // '8'
	"1001",  // '9'

	"XXXX",  // ':'
	"XXXX",  // ';'
	"XXXX",  // '<'
	"XXXX",  // '='
	"XXXX",  // '>'
	"XXXX",  // '?'
	"XXXX",  // '@'

	"1010",  // 'A'
	"1011",  // 'B'
	"1100",  // 'C'
	"1101",  // 'D'
	"1110",  // 'E'
	"1111"   // 'F'
};

int main(int argc, char *argv[]) {
	ReturnCode retVal = FLP_SUCCESS, pStatus;
	struct arg_str *ivpOpt = arg_str0("i", "ivp", "<VID:PID>", "            vendor ID and product ID (e.g 04B4:8613)");
	struct arg_str *vpOpt = arg_str1("v", "vp", "<VID:PID[:DID]>", "       VID, PID and opt. dev ID (e.g 1D50:602B:0001)");
	struct arg_str *fwOpt = arg_str0("f", "fw", "<firmware.hex>", "        firmware to RAM-load (or use std fw)");
	struct arg_str *portOpt = arg_str0("d", "ports", "<bitCfg[,bitCfg]*>", " read/write digital ports (e.g B13+,C1-,B2?)");
	struct arg_str *queryOpt = arg_str0("q", "query", "<jtagBits>", "         query the JTAG chain");
	struct arg_str *progOpt = arg_str0("p", "program", "<config>", "         program a device");
	struct arg_uint *conOpt = arg_uint0("c", "conduit", "<conduit>", "        which comm conduit to choose (default 0x01)");
	struct arg_str *actOpt = arg_str0("a", "action", "<actionString>", "    a series of CommFPGA actions");
	struct arg_lit *shellOpt  = arg_lit0("s", "shell", "                    start up an interactive CommFPGA session");
	struct arg_lit *benOpt  = arg_lit0("b", "benchmark", "                enable benchmarking & checksumming");
	struct arg_lit *rstOpt  = arg_lit0("r", "reset", "                    reset the bulk endpoints");
	struct arg_str *dumpOpt = arg_str0("l", "dumploop", "<ch:file.bin>", "   write data from channel ch to file");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "                     print this help and exit");
	struct arg_str *eepromOpt  = arg_str0(NULL, "eeprom", "<std|fw.hex|fw.iic>", "   write firmware to FX2's EEPROM (!!)");
	struct arg_str *backupOpt  = arg_str0(NULL, "backup", "<kbitSize:fw.iic>", "     backup FX2's EEPROM (e.g 128:fw.iic)\n");
	struct arg_end *endOpt   = arg_end(20);
	void *argTable[] = {
		ivpOpt, vpOpt, fwOpt, portOpt, queryOpt, progOpt, conOpt, actOpt,
		shellOpt, benOpt, rstOpt, dumpOpt, helpOpt, eepromOpt, backupOpt, endOpt
	};
	const char *progName = "flcli";
	int numErrors;
	struct FLContext *handle = NULL;
	FLStatus fStatus;
	const char *error = NULL;
	const char *ivp = NULL;
	const char *vp = NULL;
	bool isNeroCapable, isCommCapable;
	uint32 numDevices, scanChain[16], i;
	const char *line = NULL;
	uint8 conduit = 0x01;

	if ( arg_nullcheck(argTable) != 0 ) {
		fprintf(stderr, "%s: insufficient memory\n", progName);
		FAIL(1, cleanup);
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("FPGALink Command-Line Interface Copyright (C) 2012-2014 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nInteract with an FPGALink device.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL(FLP_SUCCESS, cleanup);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		fprintf(stderr, "Try '%s --help' for more information.\n", progName);
		FAIL(FLP_ARGS, cleanup);
	}

	fStatus = flInitialise(0, &error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);

	vp = vpOpt->sval[0];

	printf("Attempting to open connection to FPGALink device %s...\n", vp);
	fStatus = flOpen(vp, &handle, NULL);
	if ( fStatus ) {
		if ( ivpOpt->count ) {
			int count = 60;
			uint8 flag;
			ivp = ivpOpt->sval[0];
			printf("Loading firmware into %s...\n", ivp);
			if ( fwOpt->count ) {
				fStatus = flLoadCustomFirmware(ivp, fwOpt->sval[0], &error);
			} else {
				fStatus = flLoadStandardFirmware(ivp, vp, &error);
			}
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);

			printf("Awaiting renumeration");
			flSleep(1000);
			do {
				printf(".");
				fflush(stdout);
				fStatus = flIsDeviceAvailable(vp, &flag, &error);
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				flSleep(250);
				count--;
			} while ( !flag && count );
			printf("\n");
			if ( !flag ) {
				fprintf(stderr, "FPGALink device did not renumerate properly as %s\n", vp);
				FAIL(FLP_LIBERR, cleanup);
			}

			printf("Attempting to open connection to FPGLink device %s again...\n", vp);
			fStatus = flOpen(vp, &handle, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		} else {
			fprintf(stderr, "Could not open FPGALink device at %s and no initial VID:PID was supplied\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	printf(
		"Connected to FPGALink device %s (firmwareID: 0x%04X, firmwareVersion: 0x%08X)\n",
		vp, flGetFirmwareID(handle), flGetFirmwareVersion(handle)
	);

	if ( eepromOpt->count ) {
		if ( !strcmp("std", eepromOpt->sval[0]) ) {
			printf("Writing the standard FPGALink firmware to the FX2's EEPROM...\n");
			fStatus = flFlashStandardFirmware(handle, vp, &error);
		} else {
			printf("Writing custom FPGALink firmware from %s to the FX2's EEPROM...\n", eepromOpt->sval[0]);
			fStatus = flFlashCustomFirmware(handle, eepromOpt->sval[0], &error);
		}
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( backupOpt->count ) {
		const char *fileName;
		const uint32 kbitSize = strtoul(backupOpt->sval[0], (char**)&fileName, 0);
		if ( *fileName != ':' ) {
			fprintf(stderr, "%s: invalid argument to option --backup=<kbitSize:fw.iic>\n", progName);
			FAIL(FLP_ARGS, cleanup);
		}
		fileName++;
		printf("Saving a backup of %d kbit from the FX2's EEPROM to %s...\n", kbitSize, fileName);
		fStatus = flSaveFirmware(handle, kbitSize, fileName, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( rstOpt->count ) {
		// Reset the bulk endpoints (only needed in some virtualised environments)
		fStatus = flResetToggle(handle, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( conOpt->count ) {
		conduit = (uint8)conOpt->ival[0];
	}

	isNeroCapable = flIsNeroCapable(handle);
	isCommCapable = flIsCommCapable(handle, conduit);

	if ( portOpt->count ) {
		uint32 readState;
		char hex[9];
		const uint8 *p = (const uint8 *)hex;
		printf("Configuring ports...\n");
		fStatus = flMultiBitPortAccess(handle, portOpt->sval[0], &readState, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		sprintf(hex, "%08X", readState);
		printf("Readback:   28   24   20   16    12    8    4    0\n          %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf("  %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s\n", nibbles[*p++ - '0']);
		flSleep(100);
	}

	if ( queryOpt->count ) {
		if ( isNeroCapable ) {
			fStatus = flSelectConduit(handle, 0x00, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = jtagScanChain(handle, queryOpt->sval[0], &numDevices, scanChain, 16, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( numDevices ) {
				printf("The FPGALink device at %s scanned its JTAG chain, yielding:\n", vp);
				for ( i = 0; i < numDevices; i++ ) {
					printf("  0x%08X\n", scanChain[i]);
				}
			} else {
				printf("The FPGALink device at %s scanned its JTAG chain but did not find any attached devices\n", vp);
			}
		} else {
			fprintf(stderr, "JTAG chain scan requested but FPGALink device at %s does not support NeroProg\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( progOpt->count ) {
		printf("Programming device...\n");
		if ( isNeroCapable ) {
			fStatus = flSelectConduit(handle, 0x00, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flProgram(handle, progOpt->sval[0], NULL, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		} else {
			fprintf(stderr, "Program operation requested but device at %s does not support NeroProg\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( benOpt->count ) {
		enableBenchmarking = true;
	}

	if ( actOpt->count ) {
		printf("Executing CommFPGA actions on FPGALink device %s...\n", vp);
		if ( isCommCapable ) {
			uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
				pStatus = parseLine(handle, actOpt->sval[0], &error);
				CHECK_STATUS(pStatus, pStatus, cleanup);
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --program?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Action requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( dumpOpt->count ) {
		const char *fileName;
		unsigned long chan = strtoul(dumpOpt->sval[0], (char**)&fileName, 10);
		FILE *file = NULL;
		const uint8 *recvData;
		uint32 actualLength;
		if ( *fileName != ':' ) {
			fprintf(stderr, "%s: invalid argument to option -l|--dumploop=<ch:file.bin>\n", progName);
			FAIL(FLP_ARGS, cleanup);
		}
		fileName++;
		printf("Copying from channel %lu to %s", chan, fileName);
		file = fopen(fileName, "wb");
		CHECK_STATUS(!file, FLP_CANNOT_SAVE, cleanup);
		sigRegisterHandler();
		fStatus = flSelectConduit(handle, conduit, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		fStatus = flReadChannelAsyncSubmit(handle, (uint8)chan, 22528, NULL, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		do {
			fStatus = flReadChannelAsyncSubmit(handle, (uint8)chan, 22528, NULL, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fwrite(recvData, 1, actualLength, file);
			printf(".");
		} while ( !sigIsRaised() );
		printf("\nCaught SIGINT, quitting...\n");
		fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		fwrite(recvData, 1, actualLength, file);
		fclose(file);
	}

	if ( shellOpt->count ) {
		printf("\nEntering CommFPGA command-line mode:\n");
		if ( isCommCapable ) {
		   uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
        int coordinates_done=0;
				int first_32_bits_done=0;
				int second_32_bits_done=0;
				char *encrypted_states1;
				char *encrypted_states2;
				do {
          int iter = 0;
          while(true) {
            if(iter==64) break;
  					line = "r0 4";
  					if ( line && line[0] && line[0] != 'q' ) {
              if(coordinates_done==1)
  						{
  							if(first_32_bits_done==0)
  							{
  								line = concat("w1 ",encrypted_states1);
                  printf("%s\n", line);
  								add_history(line);
  								pStatus = parseLine(handle, line, &error);
  								CHECK_STATUS(pStatus, pStatus, cleanup);
  								line = "r0 4";
                  printf("%s\n",line);
  								add_history(line);
  								pStatus = parseLine(handle, line, &error);
  								CHECK_STATUS(pStatus, pStatus, cleanup);
  								char *decrypted_data=decrypt(f2hData);
  								printf("%s\n", decrypted_data);
                  if(strcmp(decrypted_data,"00100001001000010010000100100001")==0) /** check ack1 is as expected or not**/
  									{first_32_bits_done=1;
  									printf("%s\n", "32 bit done");
  								}
  							}
  							else if(second_32_bits_done==0)
  							{

  									// char *encrypted_states=encrypt(encrypted_states2);	/** take second_half of 64 bits **/
  									line = concat("w1 ",encrypted_states2);
  									add_history(line);
  									printf("%s\n",line );
  									pStatus = parseLine(handle, line, &error);
  									CHECK_STATUS(pStatus, pStatus, cleanup);
  									line = "r0 4";
  									add_history(line);
  									printf("%s\n",line );
  									pStatus = parseLine(handle, line, &error);
  									CHECK_STATUS(pStatus, pStatus, cleanup);
  									char *decrypted_data=decrypt(f2hData);
  									printf("%s\n", decrypted_data);
  									if(strcmp(decrypted_data,"00100001001000010010000100100001")==0) /** check ack1 is as expected or not**/
  										{second_32_bits_done=1;
  											printf("%s\n","second 32 bits done" );

  										}
  							}
  							else if(second_32_bits_done==1){
                  char *encrypted_ack2=encrypt("00000000000000000000000000000001");

  								line=concat("w1 ",encrypted_ack2); /** encrypted ack2 by host2**/
  								add_history(line);
  								printf("%s\n",line );
  								pStatus = parseLine(handle, line, &error);
  								CHECK_STATUS(pStatus, pStatus, cleanup);
  								// free((void*)line);
  								coordinates_done=0;
  								first_32_bits_done=0;
  								second_32_bits_done=0;

  							}

  						}
  						else if(coordinates_done==0)
  						{
  							line="r0 4";
  							add_history(line);
                printf("%s\n",line);
                pStatus = parseLine(handle, line, &error);
                char *decrypted_data=decrypt(f2hData);
  							char *coordiantes= decrypted_data;
  							char *encrypted_data=encrypt(decrypted_data);
  							printf("%s\n", f2hData);
  							printf("%s\n",decrypted_data);

  							line=concat("w1 ", encrypted_data);
  							add_history(line);
                printf("%s\n",line);
                pStatus = parseLine(handle, line, &error);
  							CHECK_STATUS(pStatus, pStatus, cleanup);
  							line="r0 4";
  							add_history(line);
                printf("%s\n",line);
  							pStatus = parseLine(handle, line, &error);
  							CHECK_STATUS(pStatus, pStatus, cleanup);
  							decrypted_data=decrypt(f2hData);
  							printf("%s\n", f2hData);
  							printf("%s\n", decrypted_data);
                if(strcmp(decrypted_data,"00100001001000010010000100100001") ==0)  /** ack1 from fpga right or not**/
  							{
                  char *encrypted_ack2=encrypt("00000000000000000000000000000001");
                  line=concat("w1 ",encrypted_ack2);
                	add_history(line);
                  printf("%s\n",line);
  								pStatus = parseLine(handle, line, &error);
  								CHECK_STATUS(pStatus, pStatus, cleanup);
  								coordinates_done=1;
                  printf("coordinates done\n");
  								char *encrypted_states;
                  encrypted_states= get_answer(coordiantes);
                  printf("got decrypted states\n");
  								encrypted_states1=substring_1(encrypted_states,1,8);
                	encrypted_states2=substring_1(encrypted_states,9,8);
                }
  						}

  					}
          }
				}while (true);
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --xsvf?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Shell requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

cleanup:
	free((void*)line);
	flClose(handle);
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		flFreeError(error);
	}
	return retVal;
}
