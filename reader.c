#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define UTF8_MAX_SIZE 24

//Struct definitions
struct utf8_char {
  unsigned char bytes[6];
  short size;
};
struct byte_struct {
  unsigned b_0:1;
  unsigned b_1:1;
  unsigned b_2:1;
  unsigned b_3:1;
  unsigned b_4:1;
  unsigned b_5:1;
  unsigned b_6:1;
  unsigned b_7:1;
};
struct dict_entry {
  void *key;
  void *value;
  struct dict_entry *next;
};
struct dict {
  struct dict_entry *head;
  int (*keyCompare)(void *, void*);
  void (*print)(void *);
};
struct linkedList_entry {
  void *value;
  struct linkedList_entry *next;
};
struct linkedList {
  struct linkedList_entry *head;
  //int (*valueCompare)(void *, void*);
};

//Function definitions
void dict_entry_add(struct dict *mDict, void *key, void *value, void (*valueEqualsFunc)(void *, void *));
struct dict_entry *dict_entry_init();
struct dict dict_init();
uint8_t getGammaSize(uint64_t val);
struct utf8_char *getUtf8Char(FILE *fp, bool print);
struct linkedList linkedList_init();
void linkedList_entry_add(struct linkedList *mLinkedList, void *value); 
struct linkedList_entry *linkedList_entry_init();
void printByteStruct(struct byte_struct *currByte);
void printUint8_tHex(uint8_t val);
void printUint32_tHex(uint32_t val);
void printUint64_tHex(uint64_t val);
void printUnsignedCharBits(unsigned char currChar);
void printUnsignedCharHex(unsigned char currChar);
void printUtf8Char(struct utf8_char *utf8Char);
void printUtf8CharBits(struct utf8_char *utf8Char);
int utf8_char_compare(struct utf8_char *a, struct utf8_char *b);
void utf8_char_dict_compare_valueEqualsFunc(void *a, void *b);
int utf8_char_dict_print(struct dict *mDict);

int main(int argc, char**argv){
  FILE *f;
  uint8_t currSize;
  struct utf8_char *currUtf8;
  int *currValue;
  struct dict mDict;
  struct dict_entry *currEntry;
  struct dict_entry *currEntryInner;

  if((f = fopen(argv[1], "rb")) == NULL){
    printf("Error #0");
    exit(EXIT_FAILURE);
  }

  mDict = dict_init();
  mDict.keyCompare = utf8_char_compare;
  mDict.print = utf8_char_dict_print;

  while(!feof(f)){
    currUtf8 = getUtf8Char(f, false);
    if(!(currValue = (int *)malloc(sizeof(int)))){
      printf("Error #1");
      exit(EXIT_FAILURE);      
    }
    *currValue = 1;
    
    dict_entry_add(&mDict, currUtf8, currValue, utf8_char_dict_compare_valueEqualsFunc);
  }
  
  //  mDict.print(&mDict);

  if(fclose(f) != 0){
    printf("Error #1");
    exit(EXIT_FAILURE);
  }

  currEntry = mDict.head;
  while(currEntry != NULL){
    currValue = currEntry->value;
    printf("%d\n", *currValue);

    currEntry = currEntry->next;
  }

  return EXIT_SUCCESS;
}
void dict_entry_add(struct dict *mDict, void *key, void *value, void (*valueEqualsFunc)(void *, void *)){
  struct dict_entry *prevEntry;
  struct dict_entry *currEntry;
  struct dict_entry *newEntry;

  bool done;
  int compareRes;
  if(mDict != NULL && key != NULL && value != NULL){
    newEntry = dict_entry_init();
    newEntry->key = key;
    newEntry->value = value;
    
    if(!mDict->head){
      mDict->head = newEntry;
    } else {
      done = false;
      currEntry = mDict->head;
      prevEntry = NULL;
      while(currEntry != NULL && !done){
	compareRes = mDict->keyCompare(newEntry->key, currEntry->key);
	if(compareRes == 1){
	  if(currEntry->next == NULL){
	    currEntry->next = newEntry;
	    done = true;
	  }
	} else if(compareRes == -1){
	  newEntry->next = currEntry;
	  if(prevEntry == NULL){
	    mDict->head = newEntry;
	  } else {
	    prevEntry->next = newEntry;
	  }	  
	  done = true;
	} else if(compareRes == 0){
	  valueEqualsFunc(newEntry->value, currEntry->value);
	  done = true;
	} else {
	  done = true;
	}

	prevEntry = currEntry;
	currEntry = currEntry->next;
      }
    }
  }
}
struct dict_entry *dict_entry_init(){
  struct dict_entry *result;
  
  if(!(result = (struct dict_entry *)malloc(sizeof(struct dict_entry)))){
    printf("Error: dict_entry_init() - #1\n");
    exit(EXIT_FAILURE);
  }
  
  result->key = NULL;
  result->value = NULL;
  result->next = NULL;

  return result;
}
struct dict dict_init(){
  struct dict result;

  result.head = NULL;
  result.keyCompare = NULL;
  result.print = NULL;

  return result;
}
uint8_t getGammaSize(uint64_t val){
  uint32_t shift;
  uint32_t valTmp;
  uint8_t size;
  bool done;
 
  valTmp = val << (sizeof(uint32_t)*8 - UTF8_MAX_SIZE);

  size = UTF8_MAX_SIZE;
  shift = ~0;
  done = false;
  while(shift > 0 && !done){
    shift >>= 1;
    if(valTmp > shift || size < 1){
      done = true;
    } else {
      size--;
    }
  }

  size = size*2 - 1;

  return size;
}
struct utf8_char *getUtf8Char(FILE *fp, bool print){
  struct utf8_char *utf8Char;
  unsigned char currByte;
  unsigned int currPos;
  bool encodingError;
  bool done;
  
  if(!(utf8Char = (struct utf8_char *)malloc(sizeof(struct utf8_char)))){
    printf("Error: getUtf8Char() - #1\n");
    exit(EXIT_FAILURE);
  }

  done = false;
  encodingError = false;

  utf8Char->size = 0;
  currPos = 0;
  if(fp && !feof(fp)){
    while(!done){
      currByte = getc(fp);
    
      if(!feof(fp)){
	if(print){
	  printUnsignedCharBits(currByte);
	  printf("\n");
	}
	utf8Char->bytes[currPos] = currByte;
	if(currByte < 0x80){ //0xxx xxxx
	  if(currPos != 0){
	    encodingError = true;
	  } else {
	    utf8Char->size = 1;
	    done = true;
	  }
	} else if(currByte < 0xBF){ //10xx xxxx
	  if(currPos == 0){
	    encodingError = true;
	  } else {
	    if(currPos == utf8Char->size - 1){
	      done = true;
	    }
	  }
	} else if(currByte < 0xFE){ //110x xxxx - 1111 1101
	    if(currPos != 0){
	      encodingError = true;
	    } else {
	      if(currByte < 0xE0){ //110x xxxx
		utf8Char->size = 2;
	      } else if(currByte < 0xF0){ //1110 xxxx
		utf8Char->size = 3;
	      } else if(currByte < 0xF8){ //1111 0xx
		utf8Char->size = 4;
	      } else if(currByte < 0xFC){ //1111 10xx
		utf8Char->size = 5;
	      } else { //1111 110x
		utf8Char->size = 6;
	      }	
	    }
	} else { //1111 1110 - 1111 1111, invalid UTF-8 (for UTF-16)
	  encodingError = true;
	}
      } else { 
	if(utf8Char->size != 0 && !done){
	  encodingError = true;
	} 
	 
	done = true;
      }

      if(encodingError){
	utf8Char->size = -1;
	done = true;
      }

      currPos++;
    }
  }
  
  return utf8Char;
}
struct linkedList_entry *linkedList_entry_init(){
  struct linkedList_entry *newEntry;
  
  if(!(newEntry = (struct linkedList_entry *)malloc(sizeof(struct linkedList_entry)))){
    printf("Error: linkedList_entry_init() - #1\n");
    exit(EXIT_FAILURE);    
  }

  return newEntry;
}
struct linkedList linkedList_init(){
  struct linkedList newLinkedList;

  newLinkedList.head = NULL;

  return newLinkedList;
}
void printByteStruct(struct byte_struct *currByte){
  int i;
  if(currByte == NULL){
    printf("NULL\n");
  } else {
    printf("%d%d%d%d%d%d%d%d", currByte->b_0, currByte->b_1, currByte->b_2, currByte->b_3, currByte->b_4, currByte->b_5, currByte->b_6, currByte->b_7);
  }
}
void printUint8_tHex(uint8_t val){
  printUnsignedCharHex(val);
}
void printUint32_tHex(uint32_t val){
  printUnsignedCharHex(val / 0x1000000);
  printUnsignedCharHex(val / 0x10000);
  printf(" ");
  printUnsignedCharHex(val / 0x100);
  printUnsignedCharHex(val / 0x1);
}
void printUint64_tHex(uint64_t val){
  printUnsignedCharHex(val / 0x100000000000000);
  printUnsignedCharHex(val / 0x1000000000000);
  printf(" ");
  printUnsignedCharHex(val / 0x10000000000);
  printUnsignedCharHex(val / 0x100000000);
  printf(" ");
  printUnsignedCharHex(val / 0x1000000);
  printUnsignedCharHex(val / 0x10000);
  printf(" ");
  printUnsignedCharHex(val / 0x100);
  printUnsignedCharHex(val / 0x1);
}
void printUnsignedCharBits(unsigned char currChar){
  struct byte_struct byteStruct;

  byteStruct.b_0 = 0;
  byteStruct.b_1 = 0;
  byteStruct.b_2 = 0;
  byteStruct.b_3 = 0;
  byteStruct.b_4 = 0;
  byteStruct.b_5 = 0;
  byteStruct.b_6 = 0;
  byteStruct.b_7 = 0;

  if(currChar >= 0x80){
    byteStruct.b_0 = 1;
  }
  if((currChar & 0x40) == 0x40){
    byteStruct.b_1 = 1;
  }
  if((currChar & 0x20) == 0x20){
    byteStruct.b_2 = 1;
  }
  if((currChar & 0x10) == 0x10){
    byteStruct.b_3 = 1;
  }
 
  if((currChar & 0x08) == 0x08){
    byteStruct.b_4 = 1;
  }
  if((currChar & 0x04) == 0x04){
    byteStruct.b_5 = 1;
  }
  if((currChar & 0x02) == 0x02){
    byteStruct.b_6 = 1;
  }
  if((currChar & 0x01) == 0x01){
    byteStruct.b_7 = 1;
  }
  
  printByteStruct(&byteStruct);
}
void printUnsignedCharHex(unsigned char currChar){
  char hexTable[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  printf("%c%c", hexTable[currChar / 16], hexTable[currChar % 16]);
}
void printUtf8Char(struct utf8_char *utf8Char){
  int i;
  
  if(utf8Char != NULL){
    for(i = 0; i < utf8Char->size; i++){
      printf("%d->", i);
      printUnsignedCharHex(utf8Char->bytes[i]);    
    }
  }
}
void printUtf8CharBits(struct utf8_char *utf8Char){
  int i;

  if(utf8Char != NULL){
    for(i = 0; i < utf8Char->size; i++){
      printUnsignedCharHex(utf8Char->bytes[i]);
    }
  }
}
void printUtf8FileBits(FILE *fp){
  unsigned char currByte;
  bool done;
  
  done = false;

  int i =0;
  while(!done && !feof(fp)){
    currByte = getc(fp);
    
    if(!feof(fp)){
      printUnsignedCharBits(currByte);
      printf("\n");
      i++;
    }
  }
  
  printf("\n%d\n", i);
}
int utf8_char_compare(struct utf8_char *a, struct utf8_char *b){
  int result;
  int i;
  bool done;

  result = -2;
  if(a != NULL && b != NULL && a->size != 0 && b->size != 0){
    if(a->size > b->size){
      result = 1;
    } else if(a->size < b->size){
      result = -1;
    } else {
      done = false;
      for(i = 0; i < a->size && !done; i++){
	if(a->bytes[i] > b->bytes[i]){
	  result = 1;
	  done = true;
	} else if(a->bytes[i] < b->bytes[i]){
	  result = -1;
	  done = true;
	}
      }

      if(!done){
	result = 0;
      }
    }
  }
  
  return result;
}
void utf8_char_dict_compare_valueEqualsFunc(void *a, void *b){
  int *currA;
  int *currB;

  if(a != NULL){
    currA = (int *)a;
    currB = (int *)b;
    
    *currB += *currA;;
  }
}
int utf8_char_dict_print(struct dict *mDict){
  struct dict_entry *currEntry;
  struct utf8_char *currUtf8Char;

  if(mDict != NULL){
    currEntry = mDict->head;
    while(currEntry != NULL){
      currUtf8Char = (struct utf8_char *)currEntry->key;
      printUtf8CharBits(currUtf8Char);
      printf("\t%d\n", *((int *)currEntry->value));
      currEntry = currEntry->next;
    }
  }
}
