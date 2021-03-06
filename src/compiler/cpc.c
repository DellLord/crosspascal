#include "stdint.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

typedef struct {
    uint32_t codePage;     // 0x0000 - 0xffff = ansistring codepage
    uint32_t elementSize;  // 1 = ansistring, 2 = widestring and unicodestring, 4 = hugestring
	uint32_t refCount;     // 0xffffffff = widestring which is not reference counted
	uint32_t length;
} LongstringRefHeader;

#define LongstringRefHeaderSize sizeof(LongstringRefHeader)

typedef void* pasLongstring;

void CheckRefLongstring(pasLongstring str) {
	LongstringRefHeader* header;
	
	if(str == NULL)
		return;
	header = (void*)((uint32_t)(str) - LongstringRefHeaderSize);
	if(0xffffffff == header->refCount) 
		return;
   	if((header->refCount) == 0)
	 	free(&header);
}

pasLongstring CreateLongstring(uint32_t codePage, uint32_t elementSize, uint32_t length, void* data) {
	LongstringRefHeader* header;
	char* ref;
	uint32_t* zero;
	
	if(length == 0)
		return NULL;

	header = (LongstringRefHeader*)malloc(LongstringRefHeaderSize + (sizeof(uint32_t) + (length * elementSize)));
	ref = ((char*)header) + LongstringRefHeaderSize;
	header->codePage=codePage;
	header->elementSize=elementSize;
	header->refCount=1;
	header->length=length;
	if(NULL!=data)
		memcpy(ref, data, length * elementSize);
    zero = (void*)(&(((uint8_t*)ref)[length * elementSize]));
    *zero = 0;
	return ref;
}

void DecRefLongstring(pasLongstring *str) {
	LongstringRefHeader* header;
	if(*str == NULL)
		return;
	
	header = (pasLongstring)((uint32_t)(*str) - LongstringRefHeaderSize);
	if(0xffffffff == header->refCount) 
		return;
	
	if(--(header->refCount) == 0) 
	 	free(header);
   	}

void IncRefLongstring(pasLongstring *str) {
	LongstringRefHeader* header;
	
	if(*str == NULL)
		return;
    header = (LongstringRefHeader*)((uint32_t)(*str) - LongstringRefHeaderSize);
	if(0xffffffff == header->refCount) 
		return;
	header->refCount++;
}

void FreeLongstring(pasLongstring *str) {
	DecRefLongstring(str);
	*str = NULL;
}

uint32_t LengthLongstring(pasLongstring str) {
	uint32_t len;
	LongstringRefHeader* header;
	
	if(str == NULL)
		return 0;
	header = (LongstringRefHeader*)((uint32_t)(str) - LongstringRefHeaderSize);
	len = header->length;
	CheckRefLongstring(&str);
	return len;
}

void AssignLongstring(pasLongstring *target, pasLongstring newStr) {
	DecRefLongstring(target);
	*target = newStr;
	IncRefLongstring(target);
}

void UniqueLongstring(pasLongstring *target) {
	LongstringRefHeader* header;
	pasLongstring newtarget;
	
	if(target == NULL)
		return;
    header = (LongstringRefHeader*)((uint32_t)(*target) - LongstringRefHeaderSize);
	if(header->refCount == 1)
		return;

	newtarget = CreateLongstring(header->codePage, header->elementSize, header->length, *target);
	DecRefLongstring(target);
	*target = newtarget;
}

pasLongstring AddLongstring(pasLongstring left, pasLongstring right) {
	uint32_t a,b;
	LongstringRefHeader* headerA;
    LongstringRefHeader* headerB;
	char* temp;
	uint32_t elementSize;
    uint32_t v;
    pasLongstring result;
	
	a = LengthLongstring(left);
	b = LengthLongstring(right);
	if(a + b == 0)
		return NULL;
	if(b == 0)
		return left;
	if(a == 0)
		return right;

	headerA = (LongstringRefHeader*)((uint32_t)(left) - LongstringRefHeaderSize);
	headerB = (LongstringRefHeader*)((uint32_t)(right) - LongstringRefHeaderSize);

    // TODO: Codepage handling

    if(headerA->elementSize == headerB->elementSize){
        result = CreateLongstring(headerA->codePage, headerA->elementSize, a + b, NULL);
        temp = result;
        if(a!=0){
            memcpy(temp, left, a * headerA->elementSize);
            temp += a * headerA->elementSize;
        }
        if(b!=0)
            memcpy(temp, right, b * headerB->elementSize);
    }else{
        // TODO: Variable-length UTF8 and UTF16 handling

        int i;

        elementSize = (headerA->elementSize < headerB->elementSize) ? headerB->elementSize : headerA->elementSize;
        result = CreateLongstring(headerA->codePage, elementSize, a + b, NULL);

        temp = result;

        for(i = 0; i < a; i++){
          switch(headerA->elementSize){
            case 1:{
              v = ((uint8_t*)(left))[i];
              break;
            }
            case 2:{
              v = ((uint16_t*)(left))[i];
              break;
            }
            case 4:{
              v = ((uint32_t*)(left))[i];
              break;
            }
          }
          switch(elementSize){
            case 1:{
              *((uint8_t*)temp) = v;
              break;
            }
            case 2:{
              *((uint16_t*)temp) = v;
              break;
            }
            case 4:{
              *((uint32_t*)temp) = v;
              break;
            }
          }
          temp += elementSize;
        }

        for(i = 0; i < b; i++){
          switch(headerB->elementSize){
            case 1:{
              v = ((uint8_t*)(right))[i];
              break;
            }
            case 2:{
              v = ((uint16_t*)(right))[i];
              break;
            }
            case 4:{
              v = ((uint32_t*)(right))[i];
              break;
            }
          }
          switch(elementSize){
            case 1:{
              *((uint8_t*)temp) = v;
              break;
            }
            case 2:{
              *((uint16_t*)temp) = v;
              break;
            }
            case 4:{
              *((uint32_t*)temp) = v;
              break;
            }
          }
          temp += elementSize;
        }

    }

	return result;
}

