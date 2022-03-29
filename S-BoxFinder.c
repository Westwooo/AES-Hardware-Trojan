#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./SBoxSearch.h"

#define C 67
#define X 88
#define Y 89

#define MAXTABLES 32

#define SBOX 1
#define UNUSED 2
#define TRIGGER 3

#define END "INT_INTF_RIGHT_TERM_IO"

#define MAX_X 200
#define MAX_Y 200

#define BRAM 0
#define LOGIC 1
#define TILE 2

#define BRANCHES 15

//Variables representing inputs to LUT6s
int I0, I1, I2, I3, I4, I5 = 0;
int * inputs[6];

//Dictionary to hold all permutations of LUT6 INIT values
char dictionary[720 * 32][65];
int dicIndex = 0;
int length[32] = {};
int zeros[32] = {};

//INIT value of a LUT6 implemeting XOR, and array to store such LUTs
char xorPattern[65] = {48,49,49,48,49,48,48,49,49,48,48,49,48,49,49,48,49,48,48,49,48,49,49,48,48,49,49,48,49,48,48,49,49,48,48,49,48,49,49,48,48,49,49,48,49,48,48,49,48,49,49,48,49,48,48,49,49,48,48,49,48,49,49,48};
char xorLUTs[1500][30];
int xorIndex = 0;

char beginName[30];
char beginTile[30];
char endName[30];
char endTile[30];

//An array in which to hold the connections read from the JSON file
struct connection connections[1000000];
int connectionIndex = 0;

//Arrays to hold found SBoxes and final round SBoxes
struct sBox sBoxes[500];
struct sBox finalSBoxes[500];
int sBoxIndex = 0;

//2D representation of the FPGA tiles
struct tile* cartTiles[MAX_X][MAX_Y][2];
int tilesSet[MAX_X][MAX_Y][2];

//Array of closest unused Tiles to an S-Box
struct tile unusedTiles[80]; 
int unusedFrequency[80];
int unusedIndex = 0;

int followingSBoxIndexes[16][5];
int sBoxUnderExamination = 0;
int foundFollowerIndex = 0;

int testing = 0;
int searchDone = 0;

char foundTile[20];
char initialTile[30];

FILE * triggerConnections;

//Recursive methods to find and return name and tile of all LUTs following a given port
int nextLUT(char tile[], char name[], char foundNames[][30], char foundTiles[][30], int foundLUTs) {
		
	testing = 0;
		
	int lutFound = 0;
	int newConnections = 1;
	char searchName[30];
	char searchTile[30];
	char nextSearchName[30];
	char nextSearchTile[30];
	char lutTest[4];
	struct connection * branchPoints[BRANCHES][5];
	int branches = 0;
	
	//Initialise the branchPoints arrary
	for(int i=0; i<BRANCHES; i++) {
		for(int j=0; j<5; j++){
			branchPoints[i][j] = NULL;
		}
	}
	
	strcpy(searchTile, tile); 
	strcpy(searchName, name);
	
	while(newConnections > 0) {
		
		newConnections = 0;
		
		for(int cIndex = 0; cIndex < connectionIndex; cIndex++) {
				
			if(strcmp(searchTile, connections[cIndex].beginTile) == 0 && strcmp(searchName, connections[cIndex].beginName) == 0) {
				
				//TESTING
				if(testing == 1) {
					printConnection(connections[cIndex]);
				}
				
				newConnections++;
				
				for(int i = 0; i < 3; i++) {
					lutTest[i] = connections[cIndex].endName[i+2];
				}
				lutTest[3] = 0;

			
				if(strcmp(lutTest, "LUT") == 0 /*strcmp(initialTile, connections[cIndex].endTile) != 0*/) {
					
					if(testing == 1)
						printConnection(connections[cIndex]);
					
					strcpy(foundNames[foundLUTs], connections[cIndex].endName);
					strcpy(foundTiles[foundLUTs], connections[cIndex].endTile);
					//printf("%d: %s (%s)\n", foundLUTs, foundNames[foundLUTs], foundTiles[foundLUTs]);
					
					lutFound = 1;
					foundLUTs++;
					cIndex = connectionIndex;
					newConnections = 0;

				}
				
				if(newConnections == 1) {
					strcpy(nextSearchName, connections[cIndex].endName);
					strcpy(nextSearchTile, connections[cIndex].endTile);
				}
				else if (newConnections > 1) {
					//A branch has been enountered, so safe other routes accordingly
					//printf("HERE1\n");
					branchPoints[branches][newConnections - 2] = &connections[cIndex];
				}
			}
				
		}//for
		
		if(newConnections > 1) {
			branches++;
		}
		
		//TESTING
		if(testing == 1) {
			printf("--------------------------------------------------------\n");
		}
		
		strcpy(searchName, nextSearchName);
		strcpy(searchTile, nextSearchTile);
		
	}//while
	
	for(int i = branches - 1; i >= 0; i--) {
		for(int j = 0; j < 5; j++) {
			if(branchPoints[i][j] != NULL) {
				
				if(testing == 1) {
					printf("				-----BRANCH -----\n");
					printConnection(*branchPoints[i][j]);
					printf("--------------------------------------------------------\n");
				}
				foundLUTs = nextLUT(branchPoints[i][j]->endTile, branchPoints[i][j]->endName, foundNames, foundTiles, foundLUTs);
			}
		}
	}
	
	
	return foundLUTs;
}

//Recursive method to find all LUTs or MUXs following a given point
int nextLUTorMUX(char tile[], char name[], char foundNames[][30], char foundTiles[][30], int foundLUTs) {
		
	testing = 0;
		
	int lutFound = 0;
	int newConnections = 1;
	char searchName[30];
	char searchTile[30];
	char nextSearchName[30];
	char nextSearchTile[30];
	char lutTest[4];
	char muxTest[4];
	struct connection * branchPoints[BRANCHES][5];
	int branches = 0;
	
	//Initialise the branchPoints arrary
	for(int i=0; i<BRANCHES; i++) {
		for(int j=0; j<5; j++){
			branchPoints[i][j] = NULL;
		}
	}
	
	strcpy(searchTile, tile); 
	strcpy(searchName, name);
	
	while(newConnections > 0) {
		
		newConnections = 0;
		
		for(int cIndex = 0; cIndex < connectionIndex; cIndex++) {
				
			if(strcmp(searchTile, connections[cIndex].beginTile) == 0 && strcmp(searchName, connections[cIndex].beginName) == 0) {
				
				//TESTING
				if(testing == 1) {
					printConnection(connections[cIndex]);
				}
				
				newConnections++;
				
				for(int i = 0; i < 3; i++) {
					lutTest[i] = connections[cIndex].endName[i+2];
					muxTest[i] = connections[cIndex].endName[i+2];
				}
				lutTest[3] = 0;
				muxTest[3] = 0;
				//printf(lutTest);
			
				if(strcmp(lutTest, "LUT") == 0 || (strcmp(muxTest, "MUX") == 0 && strcmp(initialTile, connections[cIndex].endTile) != 0)) {
					
					if(testing == 1)
						printConnection(connections[cIndex]);
					
					strcpy(foundNames[foundLUTs], connections[cIndex].endName);
					strcpy(foundTiles[foundLUTs], connections[cIndex].endTile);
					//printf("%d: %s (%s)\n", foundLUTs, foundNames[foundLUTs], foundTiles[foundLUTs]);
					
					lutFound = 1;
					foundLUTs++;
					cIndex = connectionIndex;
					newConnections = 0;

				}
				
				if(newConnections == 1) {
					strcpy(nextSearchName, connections[cIndex].endName);
					strcpy(nextSearchTile, connections[cIndex].endTile);
				}
				else if (newConnections > 1) {
					//A branch has been enountered, so safe other routes accordingly
					//printf("HERE1\n");
					branchPoints[branches][newConnections - 2] = &connections[cIndex];
				}
			}
				
		}//for
		
		if(newConnections > 1) {
			branches++;
		}
		
		//TESTING
		if(testing == 1) {
			printf("--------------------------------------------------------\n");
		}
		
		strcpy(searchName, nextSearchName);
		strcpy(searchTile, nextSearchTile);
		
	}//while
	
	for(int i = branches - 1; i >= 0; i--) {
		for(int j = 0; j < 5; j++) {
			if(branchPoints[i][j] != NULL) {
				
				if(testing == 1) {
					printf("				-----BRANCH -----\n");
					printConnection(*branchPoints[i][j]);
					printf("--------------------------------------------------------\n");
				}
				foundLUTs = nextLUTorMUX(branchPoints[i][j]->endTile, branchPoints[i][j]->endName, foundNames, foundTiles, foundLUTs);
			}
		}
	}
	
	
	return foundLUTs;
}

//A method to extract the Cartesian coordinates of a tile
void getTileCoords(char tile[], int* x, int* y) {
	
	int yCoordFound = 0;
	int xIndex, yIndex = 0;
	char xCoordString[4], yCoordString[4];
	
	//Extract the X and Y coordinates from the tile name
	for(int i = 0; i < strlen(tile); i++) {
		
		if(tile[i] == X) {
		
			i++;
			int xIndex = i;
			
			while(tile[i] != 89) {
				xCoordString[i - xIndex] = tile[i];
				i++;
			}
			
			xCoordString[i-xIndex] = 0;
			*x = atoi(xCoordString);
			
			yCoordFound = 1;

			i++;
			yIndex = i;
		}
		
		if(yCoordFound == 1) {
			yCoordString[i - yIndex] = tile[i];
		}
	}
	yCoordString[strlen(tile) - yIndex] = 0;
	*y = atoi(yCoordString);
}

//A method to mark the relevent LUT6 on a tile as in use
void setTileLUT(struct tile * tile, char letter, int type) {
	
	switch (letter) 
	{
		//A
		case 65:
			if (tile->LUT6s[0] == 0) 
				tile->LUT6s[0] = type;
			else
				tile->LUT6s[0] = 0;
		break;
		
		//B
		case 66:
			tile->LUT6s[1] = type;
		break;
		
		//C
		case 67:
			tile->LUT6s[2] = type;
		break;
		
		//D
		case 68:
			tile->LUT6s[3] = type;
		break;
		
		//E
		case 69:
			tile->LUT6s[4] = type;
		break;
		
		//F
		case 70:
			tile->LUT6s[5] = type;
		break;
		
		//G
		case 71:
			tile->LUT6s[6] = type;
		break;
		
		//H
		case 72:
			tile->LUT6s[7] = type;
		break;
		
		default:
		break;
	}
}

//Unset LUTs A-D on the given tile
void clearBottom(struct tile * tile) {
	for(int i = 0; i < 4; i++) {
		tile->LUT6s[i] = 0;
	}
	tile->bitA = 10;
}

//Unset LUTs E-H on the given tile
void clearTop(struct tile * tile) {
	for(int i = 7; i > 3; i--) {
		tile->LUT6s[i] = 0;
	}
	tile->bitE = 10;
}

//Print the given S-Box to the supplied file
void fputSbox(struct sBox sBox, int index, FILE * file) {
	
	fputs("Index: ", file);
	fprintf(file, "%d", index);
	fputs("   - Following Index: ", file);
	fprintf(file, "%d", sBox.nextIndex);
	fputs(" - Byte: ", file);
	fprintf(file, "%d", sBox.byte); 
	fputc(10, file);
	
	for (int i = 0; i < sBox.foundLUT8s; i++) {
		fputs(sBox.LUT8s[i].name, file);
		fputc(32, file);
		for (int j = 0; j < 8; j++) {
			fputc(sBox.LUT8s[i].LUT6s[j] + 48, file);
		}
		fputc(32, file);
		if(sBox.LUT8s[i].bitA != 10)
			fputc(sBox.LUT8s[i].bitA + 48, file);
		else
			fputc(sBox.LUT8s[i].bitE + 48, file);
		fputc(10, file);
	}
	fputs("----------------------", file);
	fputc(10, file);
}

//Calculate INIT values of SBox LUTs for all input permutations
void dictionaryUpdate (int choice) {
	int Out;
	switch (choice) 
	{
		//64'hB14EDE67096C6EED - Bit 0
		case 0:
			Out =	!I0 && !I2 && !I3 && !I4 || I0 && !I1 && I2 && !I3 && !I5 || !I0 && !I1 && !I2 && I3 && I4 || 
					!I0 && I1 && I2 && !I4 || I0 && !I2 && I3 && !I4 || I1 && !I2 && !I3 && I4 || !I0 && I1 && I2 
					&& !I3 || I1 && !I2 && I3 && !I4 || I1 && !I3 && !I4 && !I5 || I0 && I1 && I2 && I3 && I5 || 
					!I1 && I2 && I3 && I4 && I5 || I0 && !I1 && !I3 && !I4 && I5 || I0 && I1 && !I2 && I3 && !I5 || 
					!I0 && I2 && I3 && !I4 && I5 || I0 && !I2 && !I3 && I4 && I5 || I0 && !I1 && I2 && !I4 && !I5;
		break;
		
		//64'h68AB4BFA8ACB7A13 - Bit 0
		case 1:
			Out = 	I1 && I2 && !I3 && I4 && !I5 || I0 && I1 && I4 && !I5 || !I1 && !I2 && I3 && !I4 && I5 || I0 && 
					!I1 && I2 && I4 && I5 || !I0 && I1 && I2 && I3 && I5 || I0 && !I3 && I5 || I0 && !I1 && !I2 && !I5 || 
					I0 && I1 && !I2 && I3 || I2 && !I3 && !I4 && I5 || !I1 && !I2 && !I3 && I4 || !I1 && I2 && I3 && !I4 
					&& !I5 || !I0 && !I1 && !I3 && !I4 && !I5 || !I0 && I1 && I2 && I3 && !I4;
		break;
		
		//64'h10BDB210C006EAB5 - Bit 0
		case 2:
			Out =	I0 && I2 && !I4 && !I5 || I1 && I2 && I3 && !I5 || I0 && !I1 && !I2 && !I3 && I4 && !I5 || !I0 && !I1 
					&& I2 && I5 || I0 && !I1 && I3 && !I4 || I0 && I1 && !I3 && I4 && I5 || !I0 && I1 && !I2 && !I3 && I4 || 
					!I0 && !I2 && !I3 && !I4 && !I5 || I0 && !I2 && I3 && !I4 && !I5 || I0 && I1 && I2 && I3 && !I4 || I0 && 
					I2 && !I3 && I4 && I5 || !I0 && !I2 && !I3 && I4 && I5 || !I1 && I2 && !I3 && !I4 && !I5;
		break;
		
		//64'h4F1EAD396F247A04 - Bit 0
		case 3:
			Out = 	!I0 && I1 && !I2 && !I3 && !I5 || I0 && !I1 && I2 && I4 && !I5 || I0 && I1 && !I2 && I5 || 
					!I0 && !I1 && I2 && !I3 && I5 || !I0 && I1 && I3 && I4 || I0 && !I2 && I3 && !I5 || !I1 && !I2 && I3 && I4 || 
					I0 && !I1 && I2 && I3 && !I4 || !I0 && I2 && I3 && !I4 && !I5 || !I0 && !I2 && I3 && !I4 && I5 || 
					I0 && !I2 && I4 && I5 || !I0 && I1 && !I2 && I4 || I0 && I1 && I3 && !I4 && I5 || I0 && !I1 && I2 && !I4 && I5 || 
					!I0 && !I1 && !I3 && !I4 && I5;
		break;
		
		//64'h7BAE007D4C53FC7D - Bit 1
		case 4:
			Out =	I1 && !I2 && !I3 && I5 || I0 && !I3 && I4 && I5 || !I1 && I3 && I4 && I5 || !I0 && !I3 && !I4 || I1 && I3 && !I4 && !I5 || 
					!I1 && I2 && !I4 && !I5 || !I0 && I1 && I2 && !I5 || I1 && !I2 && I3 && !I5 || !I0 && !I1 && !I3 && !I5 || 
					I0 && !I2 && I4 && I5 || I1 && !I2 && !I4 && !I5 || !I1 && I2 && !I3 && !I4 || !I0 && I1 && I2 && I3 && I4 || I0 && !I1 && 
					!I2 && !I3 && I4;
		break;
		
		//64'hE61A4C5E97816F7A - Bit 1
		case 5:
			Out =	!I0 && !I1 && !I2 && I4 && !I5 || I0 && I1 && I2 && I4 && !I5 || !I0 && !I1 && I3 && I4 && !I5 || I0 && !I2 && !I3 && I5 || 
					!I0 && !I1 && I2 && !I3 && I5 || !I0 && I1 && I2 && !I4 || !I0 && I1 && !I2 && I3 || I0 && !I2 && !I4 && !I5 || I1 && !I2 && 
					!I4 && I5 || I0 && !I1 && !I4 && !I5 || !I1 && !I2 && I3 && !I5 || I0 && I2 && I3 && I4 && I5 || !I1 && I2 && !I3 && !I4 && 
					!I5 || I1 && I2 && I3 && I4 && I5 || I0 && !I1 && I3 && I4 && I5;
		break;

		//64'h6A450B2EF33486B4 - Bit 1
		case 6:
			Out =	!I1 && I2 && !I3 && !I5 || I0 && !I1 && !I2 && I3 || !I0 && I1 && !I2 && !I4 && !I5 || !I0 && I1 && !I2 && !I3 || !I1 && 
					I3 && I4 && !I5 || !I1 && !I2 && I3 && !I4 && I5 || !I0 && !I2 && !I3 && I4 && I5 || I0 && !I2 && !I4 && I5 || I0 && I1 
					&& I2 && I3 && !I5 || !I0 && I1 && I2 && I3 && I4 || I0 && !I1 && I2 && I3 && I4 || I0 && I1 && !I2 && I3 && I5 || !I0 && 
					I1 && I2 && I4 && I5 || I0 && I1 && I2 && !I4 && !I5 || I0 && !I1 && I2 && !I3 && !I4;
		break;
		
		//64'hC870974094EAD8A9 - Bit 1 
		case 7:
			Out =	!I0 && !I1 && !I2 && !I3 && !I4 && !I5 || I0 && I1 && !I4 && !I5 || !I0 && I1 && !I2 && I3 && I4 && !I5 || !I0 && !I1 && 
					I2 && I3 && !I5 || !I0 && I1 && I2 && !I3 && I5 || !I1 && !I2 && I3 && !I4 && I5 || !I0 && !I2 && I3 && !I4 && I5 || I0
					&& I1 && I2 && I3 || I0 && I1 && I3 && I4 && I5 || I0 && !I3 && I4 && !I5 || !I1 && I2 && !I3 && I4 && I5 || I0 && I2 && 
					!I3 && !I5 || I1 && I2 && I3 && I4 && I5 || I1 && I2 && I3 && !I4 && !I5 || I1 && I2 && !I3 && I4 && !I5 || !I0 && !I1 && 
					I2 && I3 && !I4;
		break;
		//64'hA16387FB3B48B4C6 - Bit 2
		case 8:
			Out =	I0 && !I1 && !I2 && !I3 && !I4 || !I1 && I2 && I3 && !I5 || I0 && I1 && !I2 && I4 && !I5 || I0 && I1 && I2 
					&& !I4 || !I0 && I1 && I2 && !I3 || !I1 && !I3 && !I4 && I5 || !I0 && I1 && !I2 && I3 && !I4 || !I1 && !I2 
					&& I3 && I4 && !I5 || !I1 && !I2 && !I4 && I5 || !I1 && !I2 && !I3 && I5 || I0 && I2 && I3 && I4 && I5 || !I0 
					&& !I1 && !I2 && I3 && I4 || I0 && !I3 && !I4 && I5 || I0 && !I1 && I2 && I4 && I5 || !I0 && I1 && !I3 && !I4 
					&& !I5;
		break;
		
		//64'h23A869A2A428C424 - Bit 2
		case 9:
			Out =	I0 && !I1 && !I3 && !I4 && I5 || !I0 && I1 && !I2 && !I4 && !I5 || I0 && !I1 && I2 && !I3 || !I0 && !I1 && !I2 
					&& I3 && I5 || I0 && I1 && !I2 && I3 && !I4 && I5 || !I0 && I1 && I2 && I3 && !I4 || I0 && I1 && !I2 && !I3 && I4 
					|| !I0 && I1 && !I2 && I3 && !I5 || I0 && I1 && I2 && I3 && !I5 || I0 && !I1 && I2 && I4 || I0 && I2 && !I3 && I5 
					|| I0 && !I1 && I2 && I3 && I5 || I0 && !I1 && I3 && I4 && I5;
		break;
		
		//64'h577D64E03B0C3FFB - Bit 2
		case 10:
			Out = 	I1 && !I2 && !I3 && I4 || !I1 && !I2 && I3 && I4 || !I1 && !I4 && !I5 || !I0 && I4 && I5 || I1 && I2 && !I3 && 
					!I4 || I0 && I1 && !I2 && !I5 || !I1 && I3 && !I5 || I0 && !I1 && I2 && !I4 && I5 || !I0 && I1 && I3 && I5 || 
					!I2 && I3 && !I4 && !I5 || I0 && !I1 && I2 && !I3 && I5;
		break;
		
		//64'hAC39B6C0D6CE2EFC - Bit 2
		case 11:
			Out = !I0 && I2 && I3 && I4 && !I5 || I0 && !I1 && I3 && !I4 || !I0 && I1 && !I2 && I3 || !I1 && I2 && I3 && !I4 && I5 || 
			!I0 && !I1 && !I3 && I4 && I5 || I1 && !I3 && !I5 || I0 && I2 && I3 && I5 || I1 && I2 && !I3 && !I4 || !I1 && I2 
			&& !I3 && !I4 && !I5 || I0 && I1 && !I2 && I4 && I5 || I0 && !I1 && !I2 && I4 && !I5 || I1 && !I2 && !I4 && !I5 || I0 
			&& I1 && I2 && I3 && I4 || I0 && !I1 && I2 && I4 && I5;
		break;	
		
		//64'h109020A2193D586A - Bit 3
		case 12:
			Out =	I0 && !I1 && !I3 && !I4 || !I0 && I1 && I2 && !I4 && !I5 || I0 && I1 && !I2 && !I5 || I0 && !I1 && I2 && !I4 && I5 || I0 
					&& I1 && I2 && !I3 && I5 || !I0 && !I1 && I4 && !I5 || !I0 && !I1 && I2 && I4 || I0 && !I1 && I2 && !I3 && !I5 || !I0 && I2 
					&& I3 && !I4 && !I5 || I1 && !I2 && !I3 && I4 && !I5;
		break;
		
		//64'h2568EA2EFFA8527D - Bit 3
		case 13:
			Out =	!I0 && !I3 && !I4 && !I5 || I1 && !I2 && !I3 && !I4 || !I0 && I1 && I2 && !I3 && I4 && I5 || I3 && I4 && !I5 || I0 && !I1 && 
					I2 && !I3 || I0 && !I2 && !I4 && I5 || I0 && I1 && I4 && !I5 || !I0 && I2 && I3 && !I5 || !I0 && !I2 && I3 && I4 || I0 && I2 && 
					I3 && !I4 && I5 || I0 && !I1 && I2 && I4 || I0 && I1 && !I2 && !I3 || I1 && I2 && I3 && !I4 && I5 || I0 && !I1 && !I2 && I3 && !I5;
		break;
		
		//64'hE9DA849CF6AC6C1B - Bit 3
		case 14:
			Out =	I0 && !I1 && I2 && I3 && !I5 || !I0 && I1 && I3 && !I5 || I0 && I2 && I4 && !I5 || I0 && !I1 && I3 && I4 && !I5 || I0 && I1 && I2 
					&& I5 || I0 && !I2 && !I3 && I4 && I5 || !I0 && !I1 && !I2 && I3 && I4 && I5 || I0 && I1 && !I2 && !I3 || I1 && I2 && I4 && I5 || 
					!I0 && I1 && !I2 && !I4 && I5 || !I0 && !I1 && I2 && !I3 && I5 || !I1 && !I2 && !I3 && !I4 && !I5 || I2 && I3 && I4 && !I5 ||
					I0 && I1 && I3 && I4 && I5 || I0 && I2 && I3 && I4 && I5 || I0 && I1 && !I2 && !I4 && !I5 || I1 && !I2 && !I3 && I4 && !I5 || !I0 
					&& !I1 && I2 && !I3 && !I4;
		break;
		
		//64'h4E9DDB76C892FB1B - Bit 3
		case 15:
			Out =	I0 && !I1 && !I2 && !I3 && !I5 || I0 && !I2 && I3 && I5 || I0 && I1 && I3 && !I5 || !I0 && I1 && I2 && I3 || !I0 && !I1 && I2 && 
					!I4 || I1 && !I2 && I4 && I5 || !I1 && !I2 && !I4 && !I5 || !I0 && !I1 && I2 && !I3 || I0 && I1 && I2 && !I3 && I4 || I0 && !I1 
					&& !I3 && !I4 && I5 || !I0 && I1 && !I3 && !I4 && I5 || I0 && I1 && I3 && !I4 || I0 && I3 && !I4 && !I5 || I0 && !I2 && !I4 && !I5
					|| !I1 && !I2 && I3 && !I4 && I5 || !I0 && !I2 && !I3 && I4 && I5;
		break;
		
		//64'hC2B0F97752B8B11E - Bit 4
		case 16:
			Out =	I0 && I2 && I3 && !I4 || I0 && !I1 && !I2 && I3 && I4 || I0 && I1 && I3 && !I4 && I5 || I0 && I2 && !I3 && I4 || !I0 && 
					!I1 && I2 && !I5 || !I1 && !I3 && !I4 && I5 || I1 && I2 && I3 && I5 || !I0 && !I1 && I3 && !I4 || I0 && I1 && !I2 && !I3
					&& !I5 || !I0 && I1 && !I2 && !I3 && !I4 || !I1 && I2 && !I3 && I4 || !I0 && I1 && I2 && I3 && I4 || I0 && !I2 && !I3 &&
					!I4 && !I5 || !I0 && !I3 && !I4 && I5;
		break;
		
		//64'hF7F17A494CE30F58 - Bit 4
		case 17:
			Out =	I0 && I1 && !I2 && !I4 || !I0 && I2 && !I3 && !I4 && !I5 || !I2 && I3 && !I4 && !I5 || !I0 && !I1 && !I2 && !I3 && I5 || 
					I0 && I2 && I4 && I5 || !I0 && I1 && I2 && I4 || I0 && !I1 && I3 && I5 || !I0 && I2 && I3 && I5 || I0 && I2 && !I3 && I4 || 
					!I0 && I1 && I3 && I4 || !I0 && !I1 && I4 && I5 || !I1 && !I2 && !I3 && I4 && !I5 || !I0 && I1 && I2 && !I3 || I1 && !I2 && 
					I3 && !I5;
		break;
		
		//64'h2624B286BC48ECB4 - Bit 4
		case 18:
			Out =	!I1 && I2 && !I3 && !I4 && !I5 || I1 && I3 && !I4 && !I5 || I0 && I1 && !I2 && I4 && !I5 || !I0 && I1 && I2 && !I3 && I4 && 
					!I5 || !I1 && I2 && I3 && I4 && !I5 || I0 && !I1 && !I2 && !I4 && I5 || !I1 && I2 && I3 && !I4 && I5 || I0 && !I1 && I2 && I4 && 
					I5 || I0 && !I1 && I3 && I5 || I0 && I1 && I2 && !I4 || I0 && I2 && I3 && !I5 || !I0 && I1 && !I2 && I3 && I4 || !I0 && I1 &&
					!I2 && !I3 && I5 || !I0 && I1 && !I2 && !I4 && !I5;
		break;
		
		//64'hF210A3AECE472E53 - Bit 4
		case 19:
			Out =	I1 && !I2 && !I3 && !I4 && I5 || !I1 && !I2 && I3 && !I4 && I5 || !I0 && !I1 && I2 && I4 && I5 || I1 && I2 && I3 && I4 || I0 
					&& !I2 && I3 && !I5 || !I1 && !I2 && !I3 && !I5 || I0 && I2 && I3 && I5 || !I0 && I1 && I4 && !I5 || I0 && !I3 && !I4 && I5 || 
					!I0 && I2 && !I3 && !I4 && !I5 || I0 && !I1 && I3 && I5 || I0 && !I1 && I3 && !I4 || I1 && !I2 && I3 && !I5;
		break;
		
		//64'hF8045F7B6D98DD7F - Bit 5
		case 20:
			Out =	!I0 && !I1 && I2 && !I3 && !I5 || I0 && I1 && !I3 && I4 && !I5 || !I0 && !I2 && I3 && !I5 || I0 && !I1 && I2 && I3 && I4 || 
					!I0 && I1 && !I2 && !I3 && I4 && I5 || !I0 && I2 && !I4 || !I1 && !I3 && !I4 || I0 && I1 && !I2 && !I4 || I2 && I3 && I4 && 
					I5 || !I0 && !I2 && I3 && !I4 || I0 && I1 && !I2 && I3 && I4 || !I2 && !I3 && !I4 && !I5 || !I0 && I1 && I2 && I3 || I1 && I3 
					&& !I4 && !I5 || I0 && !I2 && !I4 && I5;
		break;

		//64'h6BC2AA4E0D787AA4 - Bit 5
		case 21:
			Out =	!I0 && I1 && !I2 && !I3 && !I4 || I0 && I2 && !I3 && !I4 && !I5 || !I0 && I2 && I3 && !I4 && !I5 || I0 && I1 && !I2 && I4 && 
					!I5 || I1 && I2 && !I3 && I4 && I5 || !I0 && I1 && I2 && I4 && I5 || I0 && !I2 && I3 && !I4 || I0 && !I1 && !I2 && I5 || I0 && 
					!I1 && I3 && !I4 || !I1 && I2 && !I3 && I4 && !I5 || !I0 && !I2 && I3 && I4 && !I5 || I0 && I1 && !I2 && I3 || !I0 && I1 && I2 
					&& !I3 && I4 || !I0 && !I1 && !I2 && I3 && I4 || I0 && !I2 && !I3 && !I4 && I5 || I0 && I1 && I3 && !I4 && I5 || I0 && !I1 && I2
					&& I3 && I5 || !I0 && I1 && I2 && !I3 && I5;
		break;

		//64'h7D8DCC4706319E08 -Bit 5
		case 22:
			Out = 	I0 && I1 && !I2 && !I4 && !I5 || !I0 && !I1 && I2 && I3 && !I4 && !I5 || !I1 && I2 && !I3 && I4 && !I5 || I0 && !I1 && !I2 && I3 
					&& !I5 || !I1 && !I2 && !I3 && !I4 && I5 || !I0 && I1 && !I4 && I5 || I0 && I1 && !I3 && I4 && I5 || !I1 && I2 && I3 && I4 && I5 || 
					!I0 && !I2 && I4 && I5 || I0 && I1 && I3 && !I4 || !I0 && I1 && !I2 && I3 || I0 && I1 && !I2 && I3 && I5 || !I0 && I1 && I2 && I3 && 
					I5 || !I0 && !I1 && !I3 && I4 && !I5;
		break;
		
		//64'h54B248130B4F256F - Bit 5
		case 23:
			Out =	!I2 && !I3 && !I5 || I0 && !I1 && I2 && !I4 && !I5 || I0 && I1 && !I2 && I3 && !I4 && I5 || !I0 && I1 && I2 && I3 && I5 || I0 && I2 
					&& !I3 && I4 && I5 || !I0 && I1 && I3 && I4 && I5 || !I0 && I1 && !I3 && !I5 || !I1 && !I2 && I4 && !I5 || !I0 && !I2 && !I4 && !I5 
					|| !I1 && !I2 && !I3 && !I4 || !I0 && !I1 && I2 && I4 && I5 || I0 && I1 && !I2 && I4 && !I5 || I0 && !I1 && !I3 && I4 && I5 || !I0 
					&& !I1 && I2 && !I3 && I5;
		break;
		
		//64'h980A3CC2C2FDB4FF - Bit 6 
		case 24:
			Out =	I0 && !I1 && !I2 && I3 && I4 && !I5 || !I1 && I2 && I3 && !I4 || !I0 && !I1 && I2 && I3 && I5 || !I3 && !I4 && !I5 || I1 && 
					!I3 && I4 && !I5 || I0 && I1 && I2 && !I5 || I1 && I2 && !I3 && !I4 || !I1 && I2 && !I3 && !I5 || I0 && I1 && I3 && I4 && I5 
					|| I1 && !I2 && I3 && !I4 && I5 || I0 && !I2 && !I3 && I4 && I5 || !I0 && !I3 && I4 && !I5 || I1 && I2 && I3 && I4 && !I5 || 
					!I0 && I1 && !I2 && I3 && !I4 || I0 && !I1 && !I2 && !I3 && I5;
		break;
		
		//64'hE4851B3BF3AB2560 - Bit 6
		case 25:
			Out =	!I0 && I1 && I2 && !I3 && !I4 && !I5 || !I0 && !I2 && I3 && !I4 && !I5 || I0 && !I1 && I2 && !I5 || I0 && !I3 && I4 && !I5 
					|| I0 && I1 && I2 && I4 || I0 && !I2 && !I4 && I5 || !I1 && I3 && I4 && !I5 || !I1 && !I3 && !I4 && I5 || !I0 && I1 && I2 && 
					I3 && I4 || !I0 && I1 && !I2 && I4 && I5 || !I0 && !I1 && I3 && !I4 && I5 || !I0 && !I1 && !I2 && !I3 && I4 || I0 && I2 && I3 
					&& I4 && I5;
		break;
	
		//64'h3F6BCB91B30DB559 - Bit 6
		case 26:
			Out =	I0 && I1 && !I2 && !I3 && !I5 || !I0 && I2 && !I3 && !I4 && !I5 || !I0 && !I2 && I3 && !I4 && !I5 || I0 && I1 && I2 && !I4 && 
					I5 || I1 && I2 && I3 && !I4 && I5 || !I0 && I1 && I2 && !I3 && I4 && I5 || !I0 && !I1 && !I2 || I0 && I2 && I3 && !I5 || I0 &&
					!I2 && I3 && I5 || I0 && !I1 && I4 && I5 || !I1 && I2 && I3 && !I5 || I0 && !I1 && I3 && I4 || I0 && !I2 && I4 && I5 || !I0 &&
					!I1 && !I3 && !I4 || I1 && !I2 && I3 && I4 && I5 || !I1 && I2 && I3 && I4 && I5 || I1 && !I2 && !I3 && I4 && !I5;
		break;
	
		//64'h21E0B83325591782 - Bit 6
		case 27:
			Out = 	I0 && I1 && I2 && !I3 && !I4 && !I5 || !I0 && !I2 && I3 && !I5 || I0 && I1 && !I2 && !I3 && I4 && !I5 || I0 && !I1 && I2 && 
					I3 && I4 || !I1 && !I3 && !I4 && I5 || I0 && I1 && I3 && !I4 && I5 || !I0 && !I1 && !I2 && I3 && I4 || !I0 && I1 && I2 && !I3 
					&& I4 || I0 && !I1 && I2 && I5 || !I0 && !I1 && I2 && I3 && !I4 || I0 && !I1 && !I2 && !I4 && !I5 || !I0 && !I1 && !I3 && I4 && 
					!I5 || I0 && I2 && !I3 && I4 && I5;
		break;
		
		//64'h5CAA2EC7BF977090 - Bit 7
		case 28:
			Out =	!I0 && !I1 && I2 && !I5 || I0 && I1 && I2 && !I3 || !I0 && I2 && I3 && !I4 && !I5 || I0 && !I3 && I4 && I5 || I0 && I3 && I4 && !I5 
					|| I1 && !I2 && I3 && I5 || !I0 && !I2 && I4 && !I5 || !I0 && I2 && I3 && I4 && I5 || I0 && !I1 && I2 && I3 && !I4 || I0 && !I1 && !I2 
					&& !I4 && I5 || !I0 && I1 && !I3 && !I4 && I5 || !I1 && !I2 && I4 && !I5 || !I1 && !I2 && !I3 && !I4 && I5;
		break;
		
		//64'hE7BAC28F866AAC82 - Bit 7
		case 29:
			Out =	I0 && !I1 && !I2 && !I3 || I0 && I1 && I2 && !I4 || I0 && I2 && I3 && !I4 && !I5 || !I0 && I1 && I2 && !I3 && I4 && !I5 || !I2 && !I3 
					&& !I4 && I5 || !I1 && I2 && !I3 && I4 && I5 || I1 && I2 && I3 && I5 || I0 && I2 && I4 && I5 || I1 && !I2 && I3 && !I4 && !I5 || I0 && 
					!I2 && !I3 && I4 || I0 && !I1 && !I2 && I4 || !I0 && I1 && !I2 && I3 && I4 || I0 && I1 && I2 && I3 || I0 && !I1 && !I3 && I4 || I0 && !I1 
					&& !I2 && I3 && I5 || !I1 && !I2 && I3 && I4 && I5;
		break;
		
		//64'h4CB3770196CA0329 - Bit 7
		case 30:
			Out =	!I0 && !I1 && !I2 && !I4 || I0 && I1 && !I2 && !I3 && !I5 || I0 && !I1 && I2 && !I3 && !I4 && !I5 || I1 && I2 && !I3 && I4 && !I5 || !I0 
					&& I1 && !I2 && I3 && I4 || !I0 && !I1 && I2 && I3 && I4 && !I5 || I0 && I1 && I2 && I4 && !I5 || !I1 && !I3 && I4 && I5 || I1 && !I2 && I3 
					&& I4 && I5 || !I0 && I1 && I3 && I5 || !I1 && I3 && !I4 && I5 || I0 && !I1 && !I2 && I3 && !I5 || I0 && I1 && I2 && !I3 && I4 || I0 && !I2 
					&& !I3 && I4 && !I5;
		break;
		
		//64'h52379DE7B844E3E1 - Bit 7
		case 31:
			Out =	!I0 && !I1 && !I2 && !I4 || I0 && I1 && I3 && I4 && !I5 || I0 && !I1 && !I2 && I4 && I5 || !I0 && I2 && I3 && I4 && I5 || I0 && I2 && !I4 
					&& !I5 || I0 && !I1 && !I3 && I5 || I1 && I2 && !I4 && !I5 || !I1 && I2 && I3 && I4 && !I5 || !I0 && I1 && !I3 && I4 && !I5 || !I0 && !I2 
					&& !I4 && I5 || I0 && I1 && I2 && !I4 && I5 || !I0 && !I2 && !I3 && I4 && I5 || I0 && I1 && I3 && !I4 && I5 || I1 && I2 && !I3 && !I4 && I5 
					|| !I0 && !I1 && I2 && I3 && I5 || !I1 && I2 && !I3 && I4 && I5 || I0 && !I1 && I3 && !I4 && !I5;
		break;
		
	}		
	if (Out == 0 && zeros[choice] == 1) {
		dictionary[dicIndex + (720 * choice)][length[choice]] = 48;
		length[choice]++;
	}
	else if (Out == 1){
		zeros[choice] = 1;
		dictionary[dicIndex + (720 * choice)][length[choice]] = 49;
		length[choice]++;
	}
}

//Swap the values at the two addresses given
void swap(int *x, int *y) { 
    int temp; 
    temp = *x; 
    *x = *y; 
    *y = temp; 
} 

//Recursively generate all permutations of the given numbers
void generatePermutations(int *a, int l, int r) { 
	int i; 
	for (int i = 0; i < 32; i++) {
		length[i] = 0;
		zeros[i] = 0;
	}
	
	if(r == l) {
		for (int i = 1; i >= 0 ; i--) {
		//BEL pin A6
		*inputs[a[0]] = i;
			for (int j = 1; j >= 0; j--) {
				//BEL pin A5
				*inputs[a[1]] = j;
				for (int k = 1; k >= 0; k--) {
					//BEL pin A4
					*inputs[a[2]] = k;
					for (int l = 1; l >=0; l--) {
						//BEL pin A3
						*inputs[a[3]] = l;
						for (int m = 1; m >= 0; m--) {
							//BEL pin A2
							*inputs[a[4]] = m;
							for (int n = 1; n>=0; n--) {
								//BEL pin A1
								*inputs[a[5]] = n;
								
								for(int i = 0; i < MAXTABLES; i++) {
									dictionaryUpdate(i);
								}						
							}
						}
					}
				}
			}
		}
		dicIndex++;
	}
	else {
		for (i = l; i <= r; i++) 
		{ 
			swap((a+l), (a+i)); 
			generatePermutations(a, l+1, r); 
			swap((a+l), (a+i)); //backtrack 
		} 
	}
}

int unusedLUTs(struct tile tile) {
	
	for(int i = 0; i < 8; i++) {
		if(tile.LUT6s[i] == 2)
			return i;
	}
	
	return -1;
}

struct tile * closestUnusedLUT(int x, int y) {
	
	struct tile * unusedTile;
	int testResult;
	int searching = 1;
	int searchOffset = 1;
		
	unusedTile = cartTiles[x][y][0];
	
	testResult = unusedLUTs(*unusedTile);

	if(testResult > -1) {
		unusedTile->LUT6s[testResult] = 4;
		return unusedTile;
	}
	
	
	
	//If there is a second tile at the same (x,y) check if it's in use
	if(tilesSet[x][y][1] == 1) {
		
		unusedTile = cartTiles[x][y][1];
		
		testResult = unusedLUTs(*unusedTile);
		
		if(testResult > -1) {
			unusedTile->LUT6s[testResult] = 4;
			return unusedTile;
		}
	}

	while(searching == 1) {
		
		searching = 0;
		
		for(int i = -searchOffset; i < searchOffset + 1; i++) {
			for(int j = -searchOffset; j < searchOffset + 1; j++) {
				if(i != 0 || j != 0) {
					if(abs(x + i) > searchOffset - 1 || abs(y + j) > searchOffset - 1) {
						if(searchOffset < MAX_Y - y && y - searchOffset >= 0 && 
						   searchOffset < MAX_X - x && x - searchOffset >= 0) {
						  
							searching = 1;
							
							unusedTile = cartTiles[x + i][y + j][0];
							testResult = unusedLUTs(*unusedTile);
							
							if(testResult > -1) {
								unusedTile->LUT6s[testResult] = 4;
								return unusedTile;
							}
						
							if(tilesSet[x + i][y + j][1] == 1) {
								unusedTile = cartTiles[x + i][y + j][1];
								
								testResult = unusedLUTs(*unusedTile);
							
								if(testResult > -1) {
									unusedTile->LUT6s[testResult] = 4;
									return unusedTile;
								}
							}
						}
					}
				}
			}
		}
		
		searchOffset++;
	}
}

int main (int argc, char *argv[]) {	
	
	//Open the json file to be processed and check success
	if(argc == 2) {
		char fileName[40] = "./";
		RouteData = fopen(strcat(fileName, argv[1]), "r");
		if(RouteData == NULL) {
			printf("Unable to open file\n");
			exit(EXIT_FAILURE);
		}
	}
	else {
		printf("Please supply a file path to be processed\n");
		exit(EXIT_FAILURE);
	}

	//Read all connections from JSON file and store in connections[]
	while(strcmp(word, "LUT_VALUES") != 0) {
		
		//Find the start of the next word
		getNextWord(word);
	
		//"attributes" indicates the begining of a connection
		if (strcmp(word, "attributes") == 0) {
		
			getNextWord(word);
		
			//Read and set the begining and end tiles and names
			connections[connectionIndex] = newConnection();
			setTileAndName(connections[connectionIndex].beginTile, connections[connectionIndex].beginName);
			setTileAndName(connections[connectionIndex].endTile, connections[connectionIndex].endName);	
			connectionIndex++;
		}
	}

	char followingNames[7][30];
	char followingTiles[7][30];

	//Open the connection dictionaries and copy into the relevent dictionary
	char nextLine[30];
	char bramConnectionDictionary[200][3][30];
	
	FILE *bramDictionary = fopen("./Dictionaries/bramConnections.txt", "r"); 
	FILE *logicDictionary = fopen("./Dictionaries/formattedLogic.txt", "r"); 
	FILE *tileDictionary = fopen("./Dictionaries/formattedTiles.txt", "r"); 
	int tileX, tileY;
	int bramIndex = 0;
	while(fgets(nextLine, 30, bramDictionary) != NULL) {
		strcpy(bramConnectionDictionary[bramIndex][BRAM], nextLine);
		fgets(nextLine, 30, logicDictionary);
		strcpy(bramConnectionDictionary[bramIndex][LOGIC], nextLine);
		bramConnectionDictionary[bramIndex][LOGIC][strlen(bramConnectionDictionary[bramIndex][LOGIC])-2] = 0;
		fgets(nextLine, 30, tileDictionary);
		strcpy(bramConnectionDictionary[bramIndex][TILE], nextLine);
		bramIndex++;
	}
	
	//Open the file to write found S-Boxes to 
	FILE * foundThem = fopen("./FPGA_S-Boxes.txt", "w");

	//Generate all possible LUT INIT values
	inputs[0] = &I0;
	inputs[1] = &I1;
	inputs[2] = &I2;
	inputs[3] = &I3;
	inputs[4] = &I4;
	inputs[5] = &I5;
	int str[] = {5,4,3,2,1,0};	
	generatePermutations(str, 0, 5);
	
	//TO DO - remove tilesWithFreeLUTs[]
	//TO DO - write general variable description
	
	struct tile sBoxTiles[2000];				//Array holding all FPGA tiles implementing S-Boxes
	int tileIndex = 0;							//Index of above array
	struct tile tilesWithFreeLUTs[10000];		//TO DO - remove as unecessary 
	int unusedTileSet = 1;
	int unusedLUTs = 0;
	int unusedTileIndex = 0;
	struct tile allTiles[10000];				//Array holding all tiles on FPGA design
	int allTileIndex = 0;						//Index of above array	
	struct tile currentTile;					//The current tile having LUT contents read 
	char tile[30];								//String to read the tile name into
	int HammingWeight = 0;						//Hamming weight of LUT being read
	int tileSet = 1;							//Boolean representing if tiles has already begun having contents read
	int XORcount = 0;							//Number of LUTs implementing XOR function -- POSSIBLY UNECESSARY
	int sBoxLUTS = 0;							//Number of LUT6s used to implement S-Boxes
	int setLUTs = 0;							//Number of LUTs in use
	
	//Cartesian related variables - may not be necessary
	int maxX, maxY = 0;
	int xCoord, yCoord = 0;

	//Print some output for the user indicating progress
	printf("FINDING S-BOX LUTS\n-------------------------\n");
	printf("Number of Connections: %d\n", connectionIndex);
	
	//Find location of LUT6s implementing SBoxes and store information in sBoxTiles[]
	/* Search the JSON file for any LUT6s used to partialy implement S-Boxes. Those found 
	   are added to sBoxTiles 
	*/
	while(next != EOF) {
		
		getNextWord(word);
		
		//If next word is tile name - Tile names are of the form "C*" where * is a letter (ascii > 57)
		if (word[0] == C && word[1] > 57) {
			tileSet = 0;
			unusedTileSet = 0;
			strcpy(tile, word);

			getTileCoords(tile, &xCoord, &yCoord);
			
			if(yCoord > maxY)
				maxY = yCoord;
			if(xCoord > maxX)
				maxX = xCoord;
			
			currentTile = createTile(tile, xCoord, yCoord);
			allTiles[allTileIndex] = currentTile;
			allTileIndex++;
		}
		//Else next word is the name of LUT
		else if (strcmp(word, "A6LUT_O6") == 0 || strcmp(word, "B6LUT_O6") == 0 || strcmp(word, "C6LUT_O6") == 0 ||
				strcmp(word, "D6LUT_O6") == 0 || strcmp(word, "E6LUT_O6") == 0 || strcmp(word, "F6LUT_O6") == 0 ||
				strcmp(word, "G6LUT_O6") == 0 || strcmp(word, "H6LUT_O6") == 0) {
		
			
			char LUTletter = word[0];
			
			//Retrieve the LUT contents and calculate the hamming weight
			getNextWord(word);
			for (int i = 0; i < strlen(word); i++) {
				if (word[i] == 49) {
					HammingWeight++;
				}
			}
			
			if (HammingWeight > 2) {
				setLUTs++;
			}
			//TO DO - remove this 
			else {
				unusedLUTs++;
				if(unusedTileSet == 0) {
					tilesWithFreeLUTs[unusedTileIndex] = currentTile;
					setTileLUT(&tilesWithFreeLUTs[unusedTileIndex], LUTletter, UNUSED);
					setTileLUT(&allTiles[allTileIndex - 1], LUTletter, UNUSED);
					unusedTileSet = 1; 
					unusedTileIndex++;
				}
				else {
					setTileLUT(&tilesWithFreeLUTs[unusedTileIndex - 1], LUTletter, UNUSED);
					setTileLUT(&allTiles[allTileIndex - 1], LUTletter, UNUSED);
				}
			}
			
			//Min Hamming weight of S-BoxLUTs is 20
			if (HammingWeight > 20) {
				
				//If the LUT is implementing XOR, then add to list of XOR LUTs
				if (strcmp(word, xorPattern) == 0) {
					int tempLength = strlen(tile);
					tile[tempLength] = LUTletter;
					tile[tempLength + 1] = 0;
					strcpy(xorLUTs[xorIndex], tile);
					tile[tempLength] = 0;
					xorIndex++;
				}	
				//Else check if S-Box LUT 
				else {
					//TESTING
					char actualWord[65];
					int startingZeros = 1;
					int wordPos = 0;
					for(int i = 0; i <= strlen(word); i++) {
						if(word[i] == 49 && startingZeros == 1) {
							startingZeros = 0;
							wordPos = 0;
							actualWord[wordPos++] = word[i]; 
						}
						else if (startingZeros != 1) {
							actualWord[wordPos++] = word[i];
						}
					}
					
					for (int i = 0; i < 720; i++){
						for(int j = 0; j < MAXTABLES; j++) {
							if (strcmp(actualWord, dictionary[i + (720 * j)]) == 0) {											
								sBoxLUTS++;
								if (tileSet == 0) {	
								
									if(LUTletter < 69) {
										if(j < 4)
											(&currentTile)->bitA = 0;
										else if(j < 8)
											(&currentTile)->bitA = 1;
										else if(j < 12)
											(&currentTile)->bitA = 2;
										else if(j < 16)
											(&currentTile)->bitA = 3;
										else if(j < 20)
											(&currentTile)->bitA = 4;
										else if(j < 24)
											(&currentTile)->bitA = 5;
										else if(j < 28)
											(&currentTile)->bitA = 6;
										else
											(&currentTile)->bitA = 7;
									}
									else {
										if(j < 4)
											(&currentTile)->bitE = 0;
										else if(j < 8)
											(&currentTile)->bitE = 1;
										else if(j < 12)
											(&currentTile)->bitE = 2;
										else if(j < 16)
											(&currentTile)->bitE = 3;
										else if(j < 20)
											(&currentTile)->bitE = 4;
										else if(j < 24)
											(&currentTile)->bitE = 5;
										else if(j < 28)
											(&currentTile)->bitE = 6;
										else
											(&currentTile)->bitE = 7;
									}
									
									sBoxTiles[tileIndex] = currentTile;
									setTileLUT(&sBoxTiles[tileIndex], LUTletter, SBOX);
									setTileLUT(&allTiles[allTileIndex - 1], LUTletter, SBOX);
									tileSet = 1;
									tileIndex++;	
								}
								else {
									setTileLUT(&sBoxTiles[tileIndex - 1], LUTletter, SBOX);
									setTileLUT(&allTiles[allTileIndex - 1], LUTletter, SBOX);
									
									if(LUTletter < 69 && (&sBoxTiles[tileIndex - 1])->bitA == 10) {
										if(j < 4)
											(&sBoxTiles[tileIndex - 1])->bitA = 0;
										else if(j < 8)
											(&sBoxTiles[tileIndex - 1])->bitA = 1;
										else if(j < 12)
											(&sBoxTiles[tileIndex - 1])->bitA = 2;
										else if(j < 16)
											(&sBoxTiles[tileIndex - 1])->bitA = 3;
										else if(j < 20)
											(&sBoxTiles[tileIndex - 1])->bitA = 4;
										else if(j < 24)
											(&sBoxTiles[tileIndex - 1])->bitA = 5;
										else if(j < 28)
											(&sBoxTiles[tileIndex - 1])->bitA = 6;
										else
											(&sBoxTiles[tileIndex - 1])->bitA = 7;
									}
									else if (LUTletter > 68 && (&sBoxTiles[tileIndex - 1])->bitE == 10) { 
										if(j < 4)
											(&sBoxTiles[tileIndex - 1])->bitE = 0;
										else if(j < 8)
											(&sBoxTiles[tileIndex - 1])->bitE = 1;
										else if(j < 12)
											(&sBoxTiles[tileIndex - 1])->bitE = 2;
										else if(j < 16)
											(&sBoxTiles[tileIndex - 1])->bitE = 3;
										else if(j < 20)
											(&sBoxTiles[tileIndex - 1])->bitE = 4;
										else if(j < 24)
											(&sBoxTiles[tileIndex - 1])->bitE = 5;
										else if(j < 28)
											(&sBoxTiles[tileIndex - 1])->bitE = 6;
										else
											(&sBoxTiles[tileIndex - 1])->bitE = 7;
									}
									
								}
							}
						}
					}
				}
			}
			HammingWeight = 0;
		}
	}
	
	//Initialise the tilesSet array
	for(int i = 0; i < MAX_X; i++) {
		for(int j = 0; j < MAX_Y; j++) {
			tilesSet[i][j][0] = 0;
			tilesSet[i][j][1] = 0;
		}
	}
	
	//Place tiles in the cartesian representation
	for(int i = 0; i < allTileIndex; i++) {
		if(tilesSet[allTiles[i].x][allTiles[i].y][0] == 0) {
			cartTiles[allTiles[i].x][allTiles[i].y][0] = &allTiles[i];
			tilesSet[allTiles[i].x][allTiles[i].y][0] = 1;
		}
		else {
			cartTiles[allTiles[i].x][allTiles[i].y][1] = &allTiles[i];
			tilesSet[allTiles[i].x][allTiles[i].y][1] = 1;
		}
	}
	
	//Format the list of LUT8s so it can be parsed to find SBoxes
	int prevSliceIndex = tileIndex;
	for(int i = 0; i < prevSliceIndex; i++) {
		if (sBoxTiles[i].LUT6s[0] == 1 && sBoxTiles[i].LUT6s[7] == 1) {
			struct tile copy = copyTile(sBoxTiles[i]);
			clearTop(&sBoxTiles[i]);
			clearBottom(&copy);
			sBoxTiles[tileIndex] = copy;
			tileIndex++;
		}
	}
	
	//Ouput useful information to the user
	printf("Set LUT6s: %d\n", setLUTs);
	printf("Tiles holding unused LUTs: %d\n", unusedTileIndex);
	printf("S-Box LUT6s found: %d\n", sBoxLUTS);
	printf("S-Box LUT8s: %d\n", tileIndex);	
	printf("\n\n");
	
	//Indicate the begining of the next stage
	printf("SORTING LUT8S INTO S-BOXES\n");
	printf("-------------------------\n");
	int setTables = 0;
	
	//Variables used to sort the LUTs into the S-Boxes they form
	char MUXtest[8];								//String to test if a MUX has been found 
	char searchName[30];							//Name of connection being searched for 
	char searchTile[30];							//Name of tile connection being searched for 
	int connectionsFound = 0;						//Number of S-Box connections found
	char foundMUXNames[6][30];						//Name of MUXs found proceeding SBoxes
	char foundMUXTiles[6][30];						//Tiles of MUXs found proceeding SBoxes
	int sBoxCount = 0;								//Number fo SBoxes found
	char connectionA[20] = "CLE_CLE_L_SITE_0_A";	//TO DO label variables
	char connectionE[20] = "CLE_CLE_L_SITE_0_E";
	
	int doubleTesting = 0;
	
	//Sort tiles into the S-Boxes to which they belong
	//TO DO - check that this is working correectly - if not fix
	for (int index = 0; index < tileIndex; index++) {
		
		connectionsFound = 0;
		
		//Check all LUTs set on the slice
		for (int lutIndex = 0; lutIndex < 4; lutIndex++) {
			
			//Check all inputs of the LUT
			for (int input = 0; input < 6; input++) {
				
				if(sBoxTiles[index].bitA != 10) {
					connectionA[18] = input+49;
					connectionA[17] = 65+lutIndex;
					connectionA[8] = sBoxTiles[index].name[3];
					strcpy(searchName, connectionA);
				}
				else {
					connectionE[18] = input+49;
					connectionE[17] = 69+lutIndex;
					connectionE[8] = sBoxTiles[index].name[3];
					strcpy(searchName, connectionE);
				}
				
				strcpy(searchTile, sBoxTiles[index].name);
				
				int FFMUXfound = 0;
				int missedConnections = 0;
				int newMUX = 1;


				//Go back from the LUTs to find the FFMUXs that are the S-Box roots
				while (FFMUXfound == 0) {
					for (int i = connectionIndex-1; i >= 0; i--) {
						if ((strcmp(connections[i].endName, searchName) == 0 && strcmp(connections[i].endTile, searchTile) == 0)) {
							missedConnections = 0;
							strcpy(searchName, connections[i].beginName);
							strcpy(searchTile, connections[i].beginTile);
							
							//check if the connection is the output of a MUX
							for (int j = 0; j < 6; j++) {
								MUXtest[j] = connections[i].beginName[j];
								if (j == 5) {
									MUXtest[j] = 0;
								}
							}

							if (strcmp(MUXtest, "FFMUX") == 0) {
								FFMUXfound = 1;

								for (int j = 0; j < connectionsFound; j++) {
									if (strcmp(foundMUXNames[j], connections[i].beginName) == 0 && strcmp(foundMUXTiles[j], connections[i].beginTile) == 0) {
										newMUX = 0;
									}
								}

								if (newMUX == 1) {
									strcpy(foundMUXNames[connectionsFound], connections[i].beginName);
									strcpy(foundMUXTiles[connectionsFound], connections[i].beginTile);
									connectionsFound++;
								}	
							}
						}
					}
					missedConnections++;
					if (missedConnections > 2) {
						break;
					}
				}

				if(connectionsFound == 6)
					break;
			}
			if(connectionsFound == 6)
				break;
			
		}
		
		int existingSbox = 0;
		
		//Create SBoxes consiting of the tiles with common roots
		for (int i = 0; i <= sBoxIndex; i++) {
			for (int j = 0; j < sBoxes[i].foundRoots; j++) {
				for (int k = 0; k < connectionsFound; k++) {
					if (strcmp(foundMUXNames[k], sBoxes[i].rootNames[j]) == 0 && strcmp(foundMUXTiles[k], sBoxes[i].rootTiles[j]) == 0 &&  sBoxes[i].foundLUT8s < 8) {	

						(&sBoxes[i])->LUT8s[sBoxes[i].foundLUT8s] = copyTile(sBoxTiles[index]);
						
						sBoxes[i].foundLUT8s++;
						if (sBoxes[i].foundLUT8s == 8) {
							sBoxCount++;
						}
						existingSbox = 1;
						break;
					}
				}
				if (existingSbox == 1)
					break;
			}
			if (existingSbox == 1)
					break;
		}
		if (existingSbox == 0) {
			sBoxes[sBoxIndex] = newSBox(sBoxTiles[index], foundMUXTiles, foundMUXNames, connectionsFound); 
			sBoxIndex++;
		}
	
		printf("\rLUT8s processed: %d / %d || S Boxes found: %d", index, tileIndex, sBoxCount);
		fflush(stdout);	
	}
	
	//TO DO - see if unecessary - Process LUT8s with unfound roots
	struct tile unconnectedTile[10];
	int unconnectedIndex = 0;
	int incompleteSBox = 0;
	for (int i = 0; i < sBoxIndex; i++) {
		if(sBoxes[i].foundLUT8s < 8 && sBoxes[i].foundLUT8s > 0) {
			printf("HERE2\n");
			if(sBoxes[i].foundLUT8s < 2) {
				unconnectedTile[unconnectedIndex] = copyTile(sBoxes[i].LUT8s[0]);
				unconnectedIndex++;	
			}
			if(sBoxes[i].foundLUT8s > 1) {
				incompleteSBox = i;
			}
		}
	}
	for(int i = 0; i < unconnectedIndex; i++) {
		addTile(&sBoxes[incompleteSBox], unconnectedTile[i]);
	}
	
	sBoxCount = 0;
	for(int i = 0; i < sBoxIndex; i++) {		
		if(sBoxes[i].foundLUT8s == 8) {
			finalSBoxes[sBoxCount] = sBoxes[i];
			sBoxCount++;
			fputSbox(sBoxes[i], i, foundThem);
		}
	}

	printf("\rLUT8s processed: %d / %d || S Boxes found: %d", tileIndex, tileIndex, sBoxCount);
	fflush(stdout);	
	printf("\n\n\n");
	

	
	//TO DO - check necessity and label
	char LUTletter;
	int updated = 1;
	int LUTfound = 0;
	char foundTile[30];
	char tileTest[10];
	int sBoxFound = 0;
	char possibleRootTiles[10][10][30];
	char possibleRootNames[10][10][30];
	int choicesPerBranch[10]; 
	int choices = 0;
	int branches = 0;
	int retracing = 0;
	int retraceCounter = 0;
	int testing = 0; 
	int afterSboxesTotal = 0;
	int MUXnumber = 0;
	char LUTtest[8];
	char nextSearchName[30];
	char nextSearchTile[30];
	
	//Output useful information to the user
	printf("SORTING S-BOXES INTO ROUNDS\n");
	printf("-------------------------\n");
	
	int noFollowerCount = 0;
	testing = 0;
	
	int finalRoundSBoxes[16];
	int finalIndex = 0;
	int count = 0;
	
	//TESTING
	int followTest = 0;
	int foundLUTs = 0;
	//char followingNames[7][30];
	//char followingTiles[7][30];
	
	int tempIndex = 0;
	char sBoxNamesFollowing[60][30];
	char sBoxTilesFollowing[60][30];
	testing = 0;
	
	//Detect the 5 S-Boxes following each S-Box in rounds 1 - 9
	for(int i = 0; i < sBoxIndex; i++) {
		for(int j = 0; j < 8; j++) {
			
			//if(finalSBoxes[i].round > 0 && finalSBoxes[i].round < 10) {
				
				foundLUTs = 0;
				tempIndex = 0;
				
				if(finalSBoxes[i].LUT8s[j].LUT6s[0] == 1 && finalSBoxes[i].LUT8s[j].bitA == 7) {
					
					
					followTest = nextLUT(finalSBoxes[i].LUT8s[j].name, "FFMUXC1_OUT1", followingNames, followingTiles, foundLUTs);
					
					
					if(followTest == 1) {
						finalSBoxes[i].round = 10;
						break;
					}
					else if(followTest == 2) {
						finalSBoxes[i].round = -1;
						break;
					}
					
					if(testing == 1) {
						printTile(finalSBoxes[i].LUT8s[j]);
					}
					
					for(int k = 0; k < followTest; k++) {
						
						if(followingNames[k][2] == 76) {
							followingNames[k][1] = 54;
							followingNames[k][7] = 54;
						}
						
						if(testing == 1) {
							printf("     %s (%s)\n ", followingNames[k], followingTiles[k]);
						}
						
						strcpy(initialTile, followingTiles[k]);
						
						tempIndex = nextLUTorMUX(followingTiles[k], followingNames[k], sBoxNamesFollowing, sBoxTilesFollowing, 0);
						for(int l = 0; l < tempIndex; l++) {
							
							if(testing == 1) {
								printf("              %s (%s) ", sBoxNamesFollowing[l], sBoxTilesFollowing[l]);
							}
							
							for(int m = 0; m < sBoxIndex; m++) {
								for(int n = 0; n < 8; n++) {
									if(strcmp(finalSBoxes[m].LUT8s[n].name, sBoxTilesFollowing[l]) == 0) {
										if((finalSBoxes[m].LUT8s[n].LUT6s[0] == 1 && sBoxNamesFollowing[l][0] < 69) ||
										   (finalSBoxes[m].LUT8s[n].LUT6s[0] == 1 && sBoxNamesFollowing[l][6] == 66)) {
											   
											if(testing == 1)
												printf("%d, ", m);
											
											
											if(finalSBoxes[i].followingSBoxes[k] == m) {
												m = sBoxIndex; 
												n = 8;
												l = tempIndex;
											}
											else
												finalSBoxes[i].followingSBoxes[k] = m;
											
										}
										else if((finalSBoxes[m].LUT8s[n].LUT6s[7] == 1 && sBoxNamesFollowing[l][0] > 68) ||
										   (finalSBoxes[m].LUT8s[n].LUT6s[0] == 1 && sBoxNamesFollowing[l][6] == 84)) {
											
											if(testing == 1)
												printf("%d, ", m);
											
											
											if(finalSBoxes[i].followingSBoxes[k] == m) {
												m = sBoxIndex; 
												n = 8;
												l = tempIndex;
											}
											else
												finalSBoxes[i].followingSBoxes[k] = m;
											
										}
									}
								}
							}
							if(testing == 1)
								printf("\n");
						}
					}
					if(testing == 1)
						printf("\n");
				}
				else if(finalSBoxes[i].LUT8s[j].LUT6s[7] == 1 && finalSBoxes[i].LUT8s[j].bitE == 7) {
					
					if(testing == 1)
						printTile(finalSBoxes[i].LUT8s[j]);
					
					
					followTest = nextLUT(finalSBoxes[i].LUT8s[j].name, "FFMUXG1_OUT1", followingNames, followingTiles, foundLUTs);
					
					
					if(followTest == 1) {
						finalSBoxes[i].round = 10;
						break;
					}
					else if(followTest == 2) {
						finalSBoxes[i].round = -1;
						break;
					}
					
					
					for(int k = 0; k < followTest; k++) {
						if(followingNames[k][2] == 76) {
							followingNames[k][1] = 54;
							followingNames[k][7] = 54;
						}
						
						if(testing == 1)
							printf("     %s (%s)\n ", followingNames[k], followingTiles[k]);
						
						strcpy(initialTile, followingTiles[k]);
						
						tempIndex = nextLUTorMUX(followingTiles[k], followingNames[k], sBoxNamesFollowing, sBoxTilesFollowing, 0);
						for(int l = 0; l < tempIndex; l++) {
							
							if(testing == 1)
								printf("              %s (%s) ", sBoxNamesFollowing[l], sBoxTilesFollowing[l]);
							
							for(int m = 0; m < sBoxIndex; m++) {
								for(int n = 0; n < 8; n++) {
									if(strcmp(finalSBoxes[m].LUT8s[n].name, sBoxTilesFollowing[l]) == 0) {
										if((finalSBoxes[m].LUT8s[n].LUT6s[0] == 1 && sBoxNamesFollowing[l][0] < 69) ||
										   (finalSBoxes[m].LUT8s[n].LUT6s[0] == 1 && sBoxNamesFollowing[l][6] == 66)) {
											
											if(testing == 1)
												printf("%d, ", m);
											
											if(finalSBoxes[i].followingSBoxes[k] == m) {
												m = sBoxIndex; 
												n = 8;
												l = tempIndex;
											}
											else		
												finalSBoxes[i].followingSBoxes[k] = m;
										}
										else if((finalSBoxes[m].LUT8s[n].LUT6s[7] == 1 && sBoxNamesFollowing[l][0] > 68) ||
												(finalSBoxes[m].LUT8s[n].LUT6s[7] == 1 && sBoxNamesFollowing[l][6] == 84)) {
											
											if(testing == 1)
												printf("%d, ", m);
											
											
											if(finalSBoxes[i].followingSBoxes[k] == m) {
												m = sBoxIndex; 
												n = 8;
												l = tempIndex;
											}
											else
												finalSBoxes[i].followingSBoxes[k] = m;
											
										}
									}
								}
							}
							if(testing == 1)
								printf("\n");
						}
					}
					if(testing == 1)
						printf("\n");
				}
			
			//}
			
		}
		printf("\rProgress: %d / 160", i);
		fflush(stdout);
	}
	printf("\rProgress: 160 / 160");
	fflush(stdout);
	
	
	
	for(int i = 0; i < sBoxIndex; i++) {
		if(finalSBoxes[i].round == 0) {
			for(int j = 0; j < 5; j++) {
				for(int k = 0; k < 5; k++) {
					if((finalSBoxes[i].followingSBoxes[j] == finalSBoxes[i].followingSBoxes[k]) && j != k)
						finalSBoxes[i].nextIndex = finalSBoxes[i].followingSBoxes[j];
				}
			}
		}
	}
	
	
	int roundCount = 0;
	for(int rounds = 10; rounds > 1; rounds--) {
		roundCount = 0;
		for(int i = 0; i < sBoxIndex; i++) {
			if(finalSBoxes[finalSBoxes[i].nextIndex].round == rounds){
				roundCount++;
				finalSBoxes[i].round = rounds - 1;
			}
		}
	}
	
	printf("\n\n");
	
	FILE * finalRoundFound = fopen("./finalRoundLUTs.txt", "w");
	fputs(argv[1], finalRoundFound);
	fputc(10, finalRoundFound);
	char temp[10];
	sprintf(temp, "%d", finalIndex * 8);
	fputs(temp, finalRoundFound);
	fputc(10, finalRoundFound);
	
	
	//Print the final round LUTs to a file, to be used by JSON_editor.c
	for(int i = 0; i < finalIndex; i++) {
		for(int j = 0; j < 8; j++) {
			
			
			fputs(finalSBoxes[finalRoundSBoxes[i]].LUT8s[j].name, finalRoundFound);
			fputc(32, finalRoundFound);
			
			for(int k = 0; k < 8; k++) {
				fputc(finalSBoxes[finalRoundSBoxes[i]].LUT8s[j].LUT6s[k] + 48, finalRoundFound);
			}
			
			fputc(10, finalRoundFound);
			
		}
	}
	
	int searching = 0;
	int finalConnection = 0;
	
	char testSearchName[40];
	char testSearchTile[40];
	char testString[20];
	
	char lastTile[20];
	
	int tilesFound = 0;
	
	//Arays to handle branches in the FPGA paths
	int pathBranches[200];
	int branchPoint[200];
	int branchIndex = 0;
	
	int wordGroups[4][4];

	int lowerIndexes[4] = {0, 0, 0, 0};
	int upperIndexes[4] = {2, 2, 2, 2};
	
	testing = 0;
	
	//Go backwards from round 1 S-Boxes until the BRAM is found
	//Go through each round one S-Box and choose one LUT, as LUTs all recieve same input
	//Follow the connection back as far as possible
	//Then use BRAM dictionary to identify BRAM bus - ADDR...
	//The number can be used to split the bytes into four groups: 
	//		- {0,4,8,12}, {1,5,9,13}, {2,6,10,14}, {3,7,11,15}
	//Further we can distinguish between the forst ans second halves of each of these groups 
	//Since bytes 0-7, are connected to a different BRAM from bytes 8-15
	//TO DO - HOW TO DISTINGUISH BETWEEN FIRST AND SECOND HALVES OF THE STATE
	for(int i = 0; i < sBoxIndex; i++) {
		if(finalSBoxes[i].round == 1) {
			for(int j = 0; j < 6; j++) {
				
				searching = 1;
				branchIndex = 0;
				tilesFound = 0;
				
				strcpy(searchTile, (&finalSBoxes[i])->LUT8s[0].name);
				
				if((&finalSBoxes[i])->LUT8s[j].bitA != 10) {
					
					if(searchTile[3] == 77)
						strcpy(searchName, "CLE_CLE_M_SITE_0_A1");
					else
						strcpy(searchName, "CLE_CLE_L_SITE_0_A1");
					
				}
				else if((&finalSBoxes[i])->LUT8s[j].bitE != 10) {
					
					if(searchTile[3] == 77)
						strcpy(searchName, "CLE_CLE_M_SITE_0_E1");
					else
						strcpy(searchName, "CLE_CLE_L_SITE_0_E1");
				}
				
				searchName[strlen(searchName)-1] = searchName[strlen(searchName)-1] + j;
				
				while(searching == 1) {
					
					searching = 0;
					
					for(int k = connectionIndex; k >= 0; k--) {
						
						if(strcmp(searchTile, connections[k].endTile) == 0 && strcmp(searchName, connections[k].endName) == 0) {
						
							if(searching == 0) {
								finalConnection = k;
								strcpy(testSearchName, connections[k].beginName);
								strcpy(testSearchTile, connections[k].beginTile);
								searching = 1;
								
								//TESTING
								if(testing ==1)
									printConnection(connections[k]);
								
								memcpy(tileTest, connections[k].beginTile, 3);
								tileTest[3] = 0;
								
								if(strcmp(tileTest, "CLE") == 0) {
									
									if(strcmp(lastTile, connections[k].beginTile) != 0) 
										tilesFound++;
									
									strcpy(lastTile, connections[k].beginTile);
								}
							}
							else {
								pathBranches[branchIndex] = k;
								branchIndex++;
							}	
						}
					}

					if(searching == 1 && tilesFound <= 2) {
						strcpy(searchName, testSearchName);
						strcpy(searchTile, testSearchTile);
					}
					if(searching == 0 || tilesFound > 2) {
						memcpy(testString, connections[finalConnection].endTile, 6);
						testString[6] = 0;
						
						if((strcmp(testString, "INT_X8") == 0 || strcmp(testString, "INT_X2") == 0) && tilesFound <= 2) {
							searching = 0;
							break;
						}
						if(branchIndex > 0) {
							
							strcpy(searchTile, connections[pathBranches[--branchIndex]].beginTile);
							strcpy(searchName, connections[pathBranches[branchIndex]].beginName);
							tilesFound--;
							searching = 1;
						}
					}
					
				}
				
				
				int tempX, tempY;
				int testX, testY;
				char bramNumber[3];
				int numberEncountered = 0;
				
				for(int k = 0; k < bramIndex; k++) {

					if(strcmp(bramConnectionDictionary[k][LOGIC], connections[finalConnection].beginName) == 0) {
						
						
						getTileCoords(connections[finalConnection].beginTile, &tempX, &tempY);
						getTileCoords(bramConnectionDictionary[k][TILE], &testX, &testY);
						
						if(tempY % 5 == testY % 5) {
							
							for(int i = 0; i < strlen(bramConnectionDictionary[k][BRAM]); i++) {
								if(bramConnectionDictionary[k][BRAM][i] < 65) {
									if(numberEncountered != 0) {
										bramNumber[numberEncountered-1] = bramConnectionDictionary[k][BRAM][i];
										numberEncountered++;
									}
									else 
										numberEncountered = 1;
								}
							}
							bramNumber[numberEncountered-1] = 0;
							
							j = 5;
							
							break;
						}
					}
				}
				
				
				
				if(j == 5) {
					//if(tempY > 175) {
					if(tempX == 8) {
						if(atoi(bramNumber) < 3) {
							wordGroups[0][lowerIndexes[0]] = i;
							lowerIndexes[0]++;
						} else if(atoi(bramNumber) < 7) {
							wordGroups[1][lowerIndexes[1]] = i;
							lowerIndexes[1]++;
						} else if(atoi(bramNumber) < 11) {
							wordGroups[2][lowerIndexes[2]] = i;
							lowerIndexes[2]++;
						} else {
							wordGroups[3][lowerIndexes[3]] = i;
							lowerIndexes[3]++;
						}
					}
					else {
						if(atoi(bramNumber) < 3) {
							wordGroups[0][upperIndexes[0]] = i;
							upperIndexes[0]++;
						} else if(atoi(bramNumber) < 7) {
							wordGroups[1][upperIndexes[1]] = i;
							upperIndexes[1]++;
						} else if(atoi(bramNumber) < 11) {
							wordGroups[2][upperIndexes[2]] = i;
							upperIndexes[2]++;
						} else {
							wordGroups[3][upperIndexes[3]] = i;
							upperIndexes[3]++;
						}
					}
				}
			}
		}
	}
	
	//Use the column membership and information from BRAM to label the first round bytes
	//We cand split the bytes into four other groups using the following S-Box indexes:
	//		- {0,5,10,15}, {1,6,11,12}, {2,7,8,13}, {3,4,9,14}
	//However these groups cannot be distinguished between 
	//From the previous step we have 2 bytes which are either 0 or 4
	//We can distinguish between these two bytes as follows:
	//We have two groups identified in the previous section:
	//		- 1: {0,4,8,12} and 2: {1,5,9,13}
	//We have then split these further into:
	//		- 1a: {0,4}, 1b{8,12} and 2a: {1,5}, 2b: {9,13}
	//Select a byte from group 1a and check if it's in the same column as either byte from 2a
	//If so the byte selected from group 1a is byte 0, else the selected byte is byte 4
	//Now we can label the two bytes 0 and 4 and label those in the same column
	//The process is repeated with groups 1b and 2b, resulting in all bytes being labelled
	int bytes[4][4];
	int isZero = 0;
	
	for(int i = 0; i < 5; i++) {
		if(finalSBoxes[wordGroups[0][0]].followingSBoxes[0] == finalSBoxes[wordGroups[1][0]].followingSBoxes[i] || finalSBoxes[wordGroups[0][0]].followingSBoxes[0] == finalSBoxes[wordGroups[1][1]].followingSBoxes[i]) {
			isZero = 1;
		}
	}
	
	if(isZero == 1) {
		bytes[0][0] = 0;
		bytes[0][1] = 4;
	}
	else {
		bytes[0][0] = 4;
		bytes[0][1] = 0;
	}
	
	for(int idIndex = 0; idIndex < 2; idIndex++) {
		for(int i = 1; i < 4; i++) {
			for(int j = 0; j < 4; j++) {
				for(int k = 0; k < 5; k++) {
					if(finalSBoxes[wordGroups[0][idIndex]].followingSBoxes[0] == finalSBoxes[wordGroups[i][j]].followingSBoxes[k]) {
					
						if(bytes[0][idIndex] == 0) {
							
							bytes[i][j] = 5*i;
						}
						else {
							switch(i) {
								case 1:
									bytes[i][j] = 9;
									break;
								case 2:
									bytes[i][j] = 14;
									break;
								case 3:
									bytes[i][j] = 3;
									break;
							}
						}	
					}
				}
			}
		}
	}
	
	int isEight = 0; 
	
	for(int i = 0; i < 5; i++) {
		if(finalSBoxes[wordGroups[0][2]].followingSBoxes[0] == finalSBoxes[wordGroups[1][2]].followingSBoxes[i] || finalSBoxes[wordGroups[0][2]].followingSBoxes[0] == finalSBoxes[wordGroups[1][3]].followingSBoxes[i]) {
			isEight = 1;
		}
	}
	
	if(isEight == 1) {
		bytes[0][2] = 8;
		bytes[0][3] = 12;
	}
	else {
		bytes[0][2] = 12;
		bytes[0][3] = 8;
	}
	
	for(int idIndex = 2; idIndex < 4; idIndex++) {
		for(int i = 1; i < 4; i++) {
			for(int j = 0; j < 4; j++) {
				for(int k = 0; k < 5; k++) {
					if(finalSBoxes[wordGroups[0][idIndex]].followingSBoxes[0] == finalSBoxes[wordGroups[i][j]].followingSBoxes[k]) {
					
						if(bytes[0][idIndex] == 8) {
							switch(i) {
								case 1:
									bytes[i][j] = 13;
									break;
								case 2:
									bytes[i][j] = 2;
									break;
								case 3:
									bytes[i][j] = 7;
									break;
							}
						}
						else {
							switch(i) {
								case 1:
									bytes[i][j] = 1;
									break;
								case 2:
									bytes[i][j] = 6;
									break;
								case 3:
									bytes[i][j] = 11;
									break;
							}
						}	
					}
				}
			}
		}
	}
	
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			finalSBoxes[wordGroups[i][j]].byte = bytes[i][j];
		}
	}
	
	struct sBox tempSBox;
	int followingTestCount;

	//Label the remaing S-Boxes 
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j ++) {
		
			tempSBox = finalSBoxes[wordGroups[i][j]];
			followingTestCount = 0;
			
			while(tempSBox.nextIndex >= 0) {
				
				followingTestCount++;
				
				switch(tempSBox.byte) {
					
					case 0:
						finalSBoxes[tempSBox.nextIndex].byte = 13;
						break;
						
					case 1:
						finalSBoxes[tempSBox.nextIndex].byte = 10;
						break;
						
					case 2: 
						finalSBoxes[tempSBox.nextIndex].byte = 7;
						break;
						
					case 3:
						finalSBoxes[tempSBox.nextIndex].byte = 0;
						break;
						
					case 4:
						finalSBoxes[tempSBox.nextIndex].byte = 1;
						break;
						
					case 5:
						finalSBoxes[tempSBox.nextIndex].byte = 14;
						break;
						
					case 6:
						finalSBoxes[tempSBox.nextIndex].byte = 11;
						break;
						
					case 7:
						finalSBoxes[tempSBox.nextIndex].byte = 4;
						break;
						
					case 8:
						finalSBoxes[tempSBox.nextIndex].byte = 5;
						break;
						
					case 9:
						finalSBoxes[tempSBox.nextIndex].byte = 2;
						break;
						
					case 10:
						finalSBoxes[tempSBox.nextIndex].byte = 15;
						break;
						
					case 11:
						finalSBoxes[tempSBox.nextIndex].byte = 8;
						break;
						
					case 12:
						finalSBoxes[tempSBox.nextIndex].byte = 9;
						break;
						
					case 13:
						finalSBoxes[tempSBox.nextIndex].byte = 6;
						break;
						
					case 14:
						finalSBoxes[tempSBox.nextIndex].byte = 3;
						break;
						
					case 15:
						finalSBoxes[tempSBox.nextIndex].byte = 12;
						break;
						
				
				}
				
				tempSBox = finalSBoxes[tempSBox.nextIndex];
			}
		}	
	}
	
	FILE * roundSBoxes = fopen("sortedS-Boxes.txt", "w");
	fputs("FINAL ROUND:", roundSBoxes);
	fputc(10, roundSBoxes);
	fputs("----------------------", roundSBoxes);
	fputc(10, roundSBoxes);
	
	//If an S-Box is followed by a round 10 S-Box it must be in round 9 and so on 
	for(int rounds = 10; rounds > 0; rounds --) {
		
		int testingRoundCount = 0;
		
		if(rounds < 10) {
			fputs("ROUND ", roundSBoxes);
			fputc(rounds + 48, roundSBoxes);
			fputc(10, roundSBoxes);
			fputs("----------------------", roundSBoxes);
			fputc(10, roundSBoxes);
		}
		
		for(int i = 0; i < sBoxIndex; i++) {
	
			if(finalSBoxes[i].round == rounds) {
				testingRoundCount++;
				fputSbox(finalSBoxes[i], i, roundSBoxes);
			}
			
		}
	}
	
	count = 0;
	
	//Any remaining S-Boxes must be used for key generation
	fputs("KEY S-BOXES:", roundSBoxes);
	fputc(10, roundSBoxes);
	fputs("----------------------", roundSBoxes);
	
	for(int i = 0; i < sBoxCount; i++) {
		if(finalSBoxes[i].round == -1) {
			fputSbox(finalSBoxes[i], i, roundSBoxes);
			count++;
		}
	}
	
	int xorAfterFinal = 0;
	int bitCount = 0;
	int byteMapping[16] = {4, 9, 14, 3, 8, 13, 2, 7, 12, 1, 6, 11, 0, 5, 10, 15};
	int nextIndex = 0;
	
	char xorTiles[128][30];
	char xorNames[128][30];
	
	char triggerInputNames[128][30];
	char triggerInputTiles[128][30];
	
	testing = 0;
	
	for(int mapIndex = 0; mapIndex < 16; mapIndex++) {
		for(int sbIndex = 0; sbIndex < sBoxIndex; sbIndex++) {
			if(finalSBoxes[sbIndex].round == 10 && finalSBoxes[sbIndex].byte == byteMapping[mapIndex]) {
					
				for(int bit = 0; bit < 8; bit++) {
					for(int i = 0; i < 8; i++) {
						if(finalSBoxes[sbIndex].LUT8s[i].bitA == bit || finalSBoxes[sbIndex].LUT8s[i].bitE == bit) {
								
							strcpy(xorTiles[bitCount], finalSBoxes[sbIndex].LUT8s[i].name);
							
							if(testing == 1) {
								printf("%d     ", bitCount);
								printf(finalSBoxes[sbIndex].LUT8s[i].name);
							}
							
							foundLUTs = 0;
							testString[0] = 0;
							
							
							
							if(finalSBoxes[sbIndex].LUT8s[i].LUT6s[0] == 1) {
								xorAfterFinal = nextLUT(finalSBoxes[sbIndex].LUT8s[i].name, "FFMUXC1_OUT1", followingNames, followingTiles, foundLUTs);
								
								if(testing == 1)
									printf("     %s (%s)\n", followingNames[0], followingTiles[0]);
								
								xorAfterFinal = nextLUT(followingTiles[0], followingNames[0], followingNames, followingTiles, 0);
							}
							else {
								xorAfterFinal = nextLUT(finalSBoxes[sbIndex].LUT8s[i].name, "FFMUXG1_OUT1", followingNames, followingTiles, foundLUTs);
								
								if(testing == 1)
									printf("     %s (%s)\n", followingNames[0], followingTiles[0]);
								
								xorAfterFinal = nextLUT(followingTiles[0], followingNames[0], followingNames, followingTiles, 0);
							}
							
							if(testing == 1)
								printf("     %s (%s)\n", followingNames[0], followingTiles[0]);
								
							strcpy(searchName, followingNames[0]);
							strcpy(searchTile, followingTiles[0]);
							searching = 1;
							
							while(searching == 1) {
								searching = 0;
								
								for(int cIndex = 0; cIndex < connectionIndex; cIndex++) {
									
									if(strcmp(searchName, connections[cIndex].beginName) == 0 && strcmp(searchTile, connections[cIndex].beginTile) == 0) {
											
										nextIndex = cIndex;
										strcpy(nextSearchName, connections[cIndex].endName);
										strcpy(nextSearchTile, connections[cIndex].endTile);
										searching = 1;
										
										
										
										memcpy(testString, connections[cIndex].beginName, 7);
										testString[7] = 0;
										
										
										if(strcmp(testString, "CLE_CLE") == 0 && (connections[cIndex].beginName[strlen(connections[cIndex].beginName) - 1] == 81 || connections[cIndex].beginName[strlen(connections[cIndex].beginName) - 2] == 81)) {
											searching = 0;
											if(testing == 1) {
												printf("               ");
												printConnection(connections[nextIndex]);
											}
											strcpy(triggerInputNames[bitCount], connections[cIndex].beginName);
											strcpy(triggerInputTiles[bitCount], connections[cIndex].beginTile);
											bitCount++;
											break;
										}
										
					
									}
									
								}
								
								
								
								strcpy(searchName, nextSearchName);
								strcpy(searchTile, nextSearchTile);
								
							}
							if(testing == 1) {
								printf("------------------------------------\n");
							}
						}
					}	
				}	
			}
		}
	}
	
	int yAverage = 0;
	int xAverage = 0;
	int tempX;
	int tempY;
	struct tile * closest;
	
	char tempTiles[5][30];
	char tempNames[5][30];
	
	char triggerOut[30] = {"CLE_CLE_L_SITE_0_A1"};
	
	int indexTest = 5;
	
	char firstLayerTriggerTiles[128][30];
	char firstLayerTriggerNames[128][30];
	
	//Decide where the trigger connections will begin
	for(int xorIndex = 0; xorIndex < bitCount; xorIndex=xorIndex+5) {
		
		if(xorIndex > bitCount - 5) 
			indexTest = bitCount  - xorIndex;
		else
			indexTest = 5;
		
		yAverage = 0;
		xAverage = 0;
		
		for(int groupIndex = 0; groupIndex < indexTest; groupIndex++) {
			
			if(testing == 1) {
				printf(triggerInputTiles[xorIndex+groupIndex]);
				printf("\n");
			}
			
			getTileCoords(triggerInputTiles[xorIndex+groupIndex], &tempX, &tempY);
			yAverage += tempY;
			xAverage += tempX;
		}
		
		yAverage = yAverage/indexTest;
		xAverage = xAverage/indexTest;
		
		closest = closestUnusedLUT(xAverage, yAverage);
		
		if(testing == 1) {
			printf("	Average coordinates: (%d,%d)\n", xAverage, yAverage);
			printf("	");
			printTile(*closest);
			printf("\n");
		}

		memcpy(testString, closest->name, 4);
		testString[4] = 0;
		
		//TO DO - check that this is the correct connection name
		if(strcmp(testString, "CLEM") == 0) {
			strcpy(triggerOut, "CLE_CLE_M_SITE_0_A1");
		} else
			strcpy(triggerOut, "CLE_CLE_L_SITE_0_A1");
		
		for(int i = 0; i < 8; i++) {
			if(closest->LUT6s[i] == 4) {
				triggerOut[strlen(triggerOut) - 2] = triggerOut[strlen(triggerOut) - 2] + i;
				closest->LUT6s[i] = 3;
			}
		}
		
		if(testing == 1) {
			printTile(*closest);
			printf("\n");
		}
		
		for(int i = 0 ; i < indexTest; i++) {
			
			strcpy(firstLayerTriggerTiles[xorIndex+i], closest->name);
			strcpy(firstLayerTriggerNames[xorIndex+i], triggerOut);
			
			if(testing == 1) {
				printf("%d    ", xorIndex+i);
				printf(triggerInputNames[xorIndex+i]);
				printf("   (%s) ---->    ", triggerInputTiles[xorIndex+i]);
				printf(triggerOut);
				printf("   (%s)\n", closest->name);
			}
			
			triggerOut[strlen(triggerOut) - 1]++;
		}
	}
	
	triggerConnections = fopen("./triggerConnections.txt", "w");
	
	printf("-----------------FIRST LAYER CONNECTIONS---------------\n");
	
	for(int i = 0; i < 128; i++) {
		printf("%d    ", i);
		printf(triggerInputNames[i]);
		printf("   (%s) ---->    ", triggerInputTiles[i]);
		printf(firstLayerTriggerNames[i]);
		printf("   (%s)\n", firstLayerTriggerTiles[i]);
	}
	
	char secondLayerInputTiles[128][30];
	char secondLayerInputNames[128][30];
	char secondLayerOutputTiles[128][30];
	char secondLayerOutputNames[128][30];
	char tempTile[30];
	char tempName[30];
	
	int newTiles = 0;
	int secondLayerIndex = 0;
	char lutName[9] = {"A5LUT_O5"};
	lutName[8] = 0;
	
	yAverage = 0;
	xAverage = 0;
	
	testing = 0;
	
	for(int i = 0; i < 128; i++) {
		
		//If we see a new tile create a new connection in the and tree
		if(strcmp(tempTile, firstLayerTriggerTiles[i]) != 0) {
			
			strcpy(tempTile, firstLayerTriggerTiles[i]);
			strcpy(tempName, firstLayerTriggerNames[i]);
			
			if(testing == 1) {
				printf(tempTile);
				printf("\n");
			}
			
			//Set the secondlayer input names and tiles
			strcpy(secondLayerInputTiles[secondLayerIndex], tempTile);
			lutName[0] = tempName[strlen(tempName) - 2];
			strcpy(secondLayerInputNames[secondLayerIndex], lutName);
			secondLayerIndex++;
			
			
			getTileCoords(tempTile, &tempX, &tempY);
			yAverage += tempY;
			xAverage += tempX;
			
			newTiles++;
			
			if(newTiles == 5 || i == 127) {
				
				yAverage = yAverage/newTiles;
				xAverage = xAverage/newTiles;
				
				closest = closestUnusedLUT(xAverage, yAverage);
				
				memcpy(testString, closest->name, 4);
				testString[4] = 0;
				
				//TO DO - check that this is the correct connection name
				if(strcmp(testString, "CLEM") == 0) {
					strcpy(triggerOut, "CLE_CLE_M_SITE_0_A1");
				} else
					strcpy(triggerOut, "CLE_CLE_L_SITE_0_A1");
				
				for(int i = 0; i < 8; i++) {
					if(closest->LUT6s[i] == 4) {
						triggerOut[strlen(triggerOut) - 2] = triggerOut[strlen(triggerOut) - 2] + i;
						closest->LUT6s[i] = 3;
					}
				}
				
				for(int j = secondLayerIndex - newTiles; j < secondLayerIndex; j++) {
					strcpy(secondLayerOutputTiles[j], closest->name);
					strcpy(secondLayerOutputNames[j], triggerOut);
					triggerOut[strlen(triggerOut) - 1]++;
				}
				
				newTiles = 0;
			}	
		}
	}
	
	printf("-----------------SECOND LAYER CONNECTIONS---------------\n");
	
	fputs("-----------------SECOND LAYER CONNECTIONS---------------", triggerConnections);
	fputc(10, triggerConnections);
	
	char triggerLutTiles[50][30];
	char triggerLutNames[50][30];
	int triggerLutIndex = 0;
	
	for(int i = 0; i < secondLayerIndex; i++) {
		printf("%d    ", i);
		printf(secondLayerInputNames[i]);
		printf("   (%s) ---->    ", secondLayerInputTiles[i]);
		printf(secondLayerOutputNames[i]);
		printf("   (%s)\n", secondLayerOutputTiles[i]);
	}
	
	tempTile[0] = 0;
	tempName[0] = 0;
	newTiles = 0;
	
	char thirdLayerInputTiles[128][30];
	char thirdLayerInputNames[128][30];
	
	char thirdLayerOutputTiles[128][30];
	char thirdLayerOutputNames[128][30];
	int thirdLayerIndex = 0;
	
	
	
	for(int i = 0; i < secondLayerIndex - 1; i++) {
		
		//If we see a new tile create a new connection in the and tree
		if(strcmp(tempTile, secondLayerOutputTiles[i]) != 0) {
			
			strcpy(tempTile, secondLayerOutputTiles[i]);
			strcpy(tempName, secondLayerOutputNames[i]);
			
			//Set the secondlayer input names and tiles
			strcpy(thirdLayerInputTiles[thirdLayerIndex], tempTile);
			lutName[0] = tempName[strlen(tempName) - 2];

			strcpy(thirdLayerInputNames[thirdLayerIndex], lutName);
			thirdLayerIndex++;
			
			
			getTileCoords(tempTile, &tempX, &tempY);
			yAverage += tempY;
			xAverage += tempX;
			
			newTiles++;
			
			if(newTiles == 5 || i == 127) {
				
				//TO DO - change hardcoded index
				
				strcpy(thirdLayerInputTiles[thirdLayerIndex], secondLayerInputTiles[25]);
				strcpy(thirdLayerInputNames[thirdLayerIndex], secondLayerInputNames[25]);
				
				getTileCoords(thirdLayerInputTiles[thirdLayerIndex], &tempX, &tempY);
				yAverage += tempY;
				xAverage += tempX;
				newTiles++;
				
				
				yAverage = yAverage/newTiles;
				xAverage = xAverage/newTiles;
				
				closest = closestUnusedLUT(xAverage, yAverage);
				
				memcpy(testString, closest->name, 4);
				testString[4] = 0;
				
				//TO DO - check that this is the correct connection name
				if(strcmp(testString, "CLEM") == 0) {
					strcpy(triggerOut, "CLE_CLE_M_SITE_0_A1");
				} else
					strcpy(triggerOut, "CLE_CLE_L_SITE_0_A1");
				
				for(int i = 0; i < 8; i++) {
					if(closest->LUT6s[i] == 4) {
						triggerOut[strlen(triggerOut) - 2] = triggerOut[strlen(triggerOut) - 2] + i;
						closest->LUT6s[i] = 3;
					}
				}
				
				//thirdLayerIndex++;
				
				for(int j = 0; j <= thirdLayerIndex; j++) {
					strcpy(thirdLayerOutputTiles[j], closest->name);
					strcpy(thirdLayerOutputNames[j], triggerOut);
					triggerOut[strlen(triggerOut) - 1]++;
				}
				
				newTiles = 0;
			}	
		}
	}
	
	printf("-----------------THIRD LAYER CONNECTIONS---------------\n");
	
	fputs("-----------------THIRD LAYER CONNECTIONS---------------", triggerConnections);
	fputc(10, triggerConnections);
	
	for(int i = 0; i < thirdLayerIndex; i++) {
		fprintf(triggerConnections, "%d     %s.%s -> %s.%s", i, thirdLayerInputTiles[i], thirdLayerInputNames[i], thirdLayerOutputTiles[i], thirdLayerOutputNames[i]);
		fputc(10, triggerConnections);
		
		strcpy(triggerLutTiles[triggerLutIndex], thirdLayerInputTiles[i]);
		strcpy(triggerLutNames[triggerLutIndex], thirdLayerInputNames[i]);
		triggerLutIndex++;
		
		//TESTING
		printf("%d    ", i);
		printf(thirdLayerInputNames[i]);
		printf("   (%s) ---->    ", thirdLayerInputTiles[i]);
		printf(thirdLayerOutputNames[i]);
		printf("   (%s)\n", thirdLayerOutputTiles[i]);
	}
	
	
	
}