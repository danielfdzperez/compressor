/**
 * Small compression program
 *
 * Autor
 * 	Daniel Fernandez Perez
 *
 * Github:
 * 	danielfdzperez
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INT_SIZE sizeof(unsigned int)*8
#define EXTENSION ".compressed"

typedef struct TNode Node;
struct TNode{
    Node *parent;
    Node *rightSon;
    Node *leftSon;
    char value;
};

typedef struct TNodePair{
    Node *node;
    float value;
}NodePair;

typedef struct TPair{
    char value;
    unsigned int code;
    unsigned int maxIndex;
}Pair;

typedef struct TList List;
struct TList{
    List *next;
    NodePair *element;
};

Node* createNode(Node *parent){
    Node* newNode = (Node *) malloc(sizeof(Node));
    newNode->parent = parent;
    newNode->rightSon = NULL;
    newNode->leftSon = NULL;
    newNode->value = 0;
    return newNode;
}

void createListElement(List *list, char value){
    list->element = (NodePair *) malloc(sizeof(NodePair));
    list->element->value = 1;
    list->element->node = (Node *) malloc(sizeof(Node));
    list->element->node->value = value;
    list->element->node->parent = NULL;
    list->element->node->rightSon = NULL;
    list->element->node->leftSon = NULL;

}

char *addExtension(char *document){
    const char *extension = EXTENSION;
    char *documentWithExtension = malloc(strlen(document)+strlen(extension)+1);
    if(documentWithExtension == NULL){
	printf("Mallox extension error\n");
	exit(EXIT_FAILURE);
    }
    strcpy(documentWithExtension, document);
    strcat(documentWithExtension, extension);
    return documentWithExtension;
}

int addSymbol(List *alphabet, char character){
    int addition = 0;
    if(alphabet->element == NULL){
	createListElement(alphabet, character);
	return 1;
    }

    if(alphabet->element->node->value == character)
	alphabet->element->value ++;
    else
	if(alphabet->next == NULL){
	    List *new = (List*) malloc(sizeof(List));
	    new->next = NULL;
	    new->element = NULL;
	    addition = addSymbol(new, character);
	    alphabet->next = new;
	}
	else
	    addition = addSymbol(alphabet->next, character);
    return addition;
}

void readFile(List *alphabet, int *totalElements, int *listLength, char *document){
    FILE *fd = NULL;
    if( !( fd = fopen(document,"r") ) ){
	printf("Error reading file %s\n",document);
	exit(EXIT_FAILURE);
    }
    else{
	char character;
	while( !ferror(fd) && !feof(fd) ){
	    character = fgetc(fd);
	    (*totalElements) ++;
	    (*listLength) += addSymbol(alphabet,character);
	}
	fclose(fd);
    }
}

void insert(List *list, NodePair **orderList){
    int i;
    for(i = 0; list->next != NULL; list = list->next,i++){
	orderList[i] = list->element;
    }
    orderList[i] = list->element;
}

void probability(NodePair **list, int listLength, int total){
    int i;
    for(i = 0; i < listLength; i++){
	list[i]->value /= total;
    }
}

void arrange(NodePair **list, int elements){
    int i,j;
    for(i=0; i < elements; i++)
	for(j=i+1; j<elements; j++)
	    if(list[i]->value < list[j]->value){
		NodePair * aux = list[i];
		list[i] = list[j];
		list[j] = aux;
	    }
}

//Genera un arbol para comprimir un archivo
void generateTree(NodePair **list, int listLength){
    if(listLength <= 1)
	return;
    arrange(list, listLength);

    NodePair* rigth = list[listLength-1];
    NodePair* left = list[listLength-2];
    NodePair* new  = (NodePair *) malloc(sizeof(NodePair));
    new->value = rigth->value + left->value;
    new->node = (Node *) malloc(sizeof(Node));
    new->node->value = 0;
    new->node->parent = NULL;
    new->node->rightSon = rigth->node;
    new->node->leftSon = left->node;
    listLength --;
    list[listLength - 1] = new;
    generateTree(list,listLength);
}

//Crea una entrada en el diccionario
void encode(int code, Node *node, int sift, Pair **dictionary, int *dictionaryIndex){
    if(node->rightSon == NULL && node->leftSon == NULL){
	Pair *new = (Pair *) malloc(sizeof(Pair*));
	new->value = node->value;
	new->code = code;
	new->maxIndex = sift;
	dictionary[*dictionaryIndex] = new;
	(*dictionaryIndex) ++;
	return;
    }

    //Derecha es 1
    if(node->rightSon != NULL){
	int rightCode = code | (1 << sift);
	encode(rightCode, node->rightSon, sift+1, dictionary, dictionaryIndex);
    }
    //Izquierda 0
    if(node->leftSon != NULL)
	encode(code, node->leftSon, sift+1, dictionary, dictionaryIndex);

}

void getCode(char character, unsigned int *code, int *indexCode, Pair **dictionary, int dictionaryLength, FILE *wfd){
    int find = 0;
    int i;
    int currentIndex = 0;


    for(i = 0; i < dictionaryLength && !find; i++)
	if(character == dictionary[i]->value){
	    i --;
	    find = 1;
	}


    while( currentIndex < dictionary[i]->maxIndex){
	if(*indexCode >=  INT_SIZE){
	    *indexCode = 0;
	    fwrite(code, sizeof(*code), 1, wfd);
	    *code = 0;
	}
	(*code) |= (   ((dictionary[i]->code & (1 << currentIndex)) >> currentIndex) << (*indexCode) );
	(*indexCode) ++;
	currentIndex ++;
    }

}

void compress(Pair **dictionary, int dictionaryLength, int totalElements, char *document){
    FILE *rfd = NULL;
    FILE *wfd = NULL;
    int i;
    unsigned int code = 0;
    int indexCode = 0;
    char *documentWithExtension = addExtension(document);

    wfd = fopen(documentWithExtension,"wb");
    fwrite(&totalElements, sizeof(totalElements), 1, wfd);
    fwrite(&dictionaryLength, sizeof(dictionaryLength), 1, wfd);
    for(i = 0; i < dictionaryLength; i++)
	fwrite(dictionary[i], sizeof(*dictionary[i]), 1, wfd);

    if( !( rfd = fopen(document,"r") ) ){
	printf("compression error with files %s\n",document);
	exit(EXIT_FAILURE);
    }
    else{
	while( !ferror(rfd) && !feof(rfd) ){
	    getCode(fgetc(rfd), &code, &indexCode, dictionary, dictionaryLength, wfd);
	}
	fclose(rfd);
	if(indexCode != 0)
	    fwrite(&code, sizeof(code), 1, wfd);
    }
    fclose(wfd);
}

void formTree(Node *node, Pair *dictionaryElement, unsigned int index){
    if(index >= dictionaryElement->maxIndex){
	node->value = dictionaryElement->value;
	return;
    }
    int branch = ((dictionaryElement->code & (1 << index)) >> index);
    index ++;
    if(branch == 1){
	if(node->rightSon == NULL){
	    Node* newNode = createNode(node);
	    node->rightSon = newNode;
	}
	formTree(node->rightSon, dictionaryElement, index);
    }
    else{
	if(node->leftSon == NULL){
	    Node* newNode = createNode(node);
	    node->leftSon = newNode;
	}
	formTree(node->leftSon, dictionaryElement, index);
    }
}

char decode(Node *node, unsigned int *code, unsigned int *codeIndex, FILE *fd){
    if(node->rightSon == NULL && node->leftSon == NULL)
	return node->value;
    char character;

    if(*codeIndex >= INT_SIZE){
	fread(code, sizeof(*code), 1, fd);
	*codeIndex = 0;
    }
    int branch = ( ( (*code) & (1 << (*codeIndex)) ) >> (*codeIndex) );
    (*codeIndex) ++;
    if(branch == 1){
	character = decode(node->rightSon, code, codeIndex, fd);
    }
    else{
	character = decode(node->leftSon, code, codeIndex, fd);
    }
    return character;
}


void decompress(char *document){
    int totalElements;
    int dictionaryLength;
    Pair *dictionaryElement;
    int i;
    Node *root;
    unsigned int code;
    unsigned int codeIndex;

    FILE *rfd;
    FILE *wfd;


    char *documentWithExtension = addExtension(document);

    if( !( rfd = fopen(documentWithExtension,"r") ) ){
	printf("decompression error with files %s\n",document);
	exit(EXIT_FAILURE);
    }
    else{
	fread(&totalElements, sizeof(totalElements), 1, rfd);
	fread(&dictionaryLength, sizeof(dictionaryLength), 1, rfd);
	dictionaryElement = (Pair *) malloc(sizeof(Pair));

	root = createNode(NULL);
	for(i=0; i < dictionaryLength; i++){
	    fread(dictionaryElement, sizeof(*dictionaryElement), 1, rfd);
	    formTree(root, dictionaryElement, 0);
	}

	wfd = fopen(document,"wb");
	codeIndex = 0;
	fread(&code, sizeof(code), 1, rfd);
	while(totalElements > 1){
	    char character = decode(root, &code, &codeIndex, rfd);
	    fwrite(&character, sizeof(character), 1, wfd);
	    totalElements --;
	}
	fclose(wfd);
	fclose(rfd);
    }
    free(documentWithExtension);
}

void startCompress(char *document){
    List *alphabet = (List*) malloc(sizeof(List));
    alphabet->next = NULL;
    alphabet->element = NULL;
    int totalElements = 0;
    int listLength = 0;
    NodePair **orderList;
    Pair **dictionary;
    int dictionaryLength = 0;


    readFile(alphabet, &totalElements, &listLength,document);
    orderList = (NodePair **) malloc(sizeof(NodePair*) * listLength );
    insert(alphabet, orderList);
    probability(orderList, listLength, totalElements);
    generateTree(orderList, listLength);
    dictionary = (Pair **) malloc(sizeof(Pair*) * listLength);
    encode(0,orderList[0]->node, 0, dictionary, &dictionaryLength);

    compress(dictionary, dictionaryLength, totalElements,document);
}

void printHelp(const char *program){
    fprintf(stderr, "\nUse: %s [option] <file name, for decompress dont type the extension %s>"
	    "\n"
	    "\n"
	    "\n\toptions"
	    "\n\t======="
	    "\n\t\t -z compress"
	    "\n\t\t -u decompress"
	    "\n\t\t -h use"
	    "\n\n", 
	    program, EXTENSION);
}



int main (int argc, char* argv[]){
    char *document = NULL;
    char option;
    int help = 0;

    while( (option = getopt(argc, argv, "z:u:h") ) != -1)
	switch(option){
	    case 'z':
		startCompress(optarg);
		break;
	    case 'u':
		decompress(optarg);
		break;
	    case 'h':
	    default:
		help = 1;
	}
    if(help || argc < 2){
	printHelp(argv[0]);
	    exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
