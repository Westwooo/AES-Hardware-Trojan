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

//Variables representing inputs to LUT6s
int I0, I1, I2, I3, I4, I5 = 0;
int * inputs[6];

//Dictionary to hold all permutations of LUT6 INIT values
char dictionary[720 * 32][65];
int dicIndex = 0;
int length[32] = {};
int zeros[32] = {};

//INIT value of a LUT6 implemeting XOR, and array to store such LUTs
char xorPattern[65] = {49,49,48,49,48,48,49,49,48,48,49,48,49,49,48,49,48,48,49,48,49,49,48,48,49,49,48,49,48,48,49,49,48,48,49,48,49,49,48,48,49,49,48,49,48,48,49,48,49,49,48,49,48,48,49,49,48,48,49,48,49,49,48};
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

//A method to find all LUTs immeadiately following a given LUT on a given TILE
void followingLUTs(char tile[], char name[], char LUTNames[][30], char LUTTiles[][30],  int *followingIndex) {	
	
	char searchTile[30];
	char searchName[30];
	
	strcpy(searchTile, tile);
	strcpy(searchName, name);
	
	int connectionFound = 1;
	
	char test[30];
	
	*followingIndex = 0;
	
	//An array representing a path through the FPGA
	int path[500];
	int pathIndex = 0;
	
	//Arays to handle branches in the FPGA paths
	int pathBranches[200];
	int branchPoint[200];
	int branchIndex = 0;
	
	int foundIndex;
	
	char isLUT[5];
	char isMUX[5];
	
	while(connectionFound == 1) {
		
		connectionFound = 0;
		
		for(int i = 0; i < connectionIndex; i++) {
			if(strcmp(connections[i].beginTile, searchTile) == 0 && strcmp(connections[i].beginName, searchName) == 0) {
				
				for(int j = 1; j < 5; j++) {
					isLUT[j-1] = connections[i].endName[j];
				}
				isLUT[4] = 0;
				
				for(int j = 2; j < 5; j++) {
					isMUX[j-2] = connections[i].endName[j];
				}
				isMUX[3] = 0;

				if(strcmp("6LUT", isLUT) == 0) {
					//printf("%s %s\n", connections[i].endTile, connections[i].endName);
					strcpy(LUTNames[*followingIndex], connections[i].endName);
					strcpy(LUTTiles[(*followingIndex)++], connections[i].endTile);
				}
				
				else if(connectionFound == 0) {
					connectionFound = 1;
					foundIndex = i;
				}
				else {
					pathBranches[branchIndex] = i;
					branchIndex++;
				}
			}
		}
		
		if(connectionFound == 0 && branchIndex > 0) {
			strcpy(searchTile, connections[pathBranches[--branchIndex]].endTile);
			strcpy(searchName, connections[pathBranches[branchIndex]].endName);
			connectionFound = 1;
		}
		else {
			strcpy(searchTile, connections[foundIndex].endTile);
			strcpy(searchName, connections[foundIndex].endName);
		}
		
	}
	
}

//A method to find all LUT or MUXs immediately following a given LUT on a given TILE
void followingLUTandMUX(char tile[], char name[], char LUTNames[][30], char LUTTiles[][30],  int *followingIndex) {	
	
	char searchTile[30];
	char searchName[30];
	
	strcpy(searchTile, tile);
	strcpy(searchName, name);
	
	int connectionFound = 1;
	
	char test[30];
	
	*followingIndex = 0;
	
	//An array representing a path through the FPGA
	int path[500];
	int pathIndex = 0;
	
	//Arays to handle branches in the FPGA paths
	int pathBranches[200];
	int branchPoint[200];
	int branchIndex = 0;
	
	int foundIndex;
	
	char isLUT[5];
	char isMUX[5];
	
	while(connectionFound == 1) {
		
		connectionFound = 0;
		
		for(int i = 0; i < connectionIndex; i++) {
			if(strcmp(connections[i].beginTile, searchTile) == 0 && strcmp(connections[i].beginName, searchName) == 0) {
				
				for(int j = 1; j < 5; j++) {
					isLUT[j-1] = connections[i].endName[j];
				}
				isLUT[4] = 0;
				
				for(int j = 2; j < 5; j++) {
					isMUX[j-2] = connections[i].endName[j];
				}
				isMUX[3] = 0;

				if(strcmp("6LUT", isLUT) == 0) {
					//printf("%s %s\n", connections[i].endTile, connections[i].endName);
					strcpy(LUTNames[*followingIndex], connections[i].endName);
					strcpy(LUTTiles[(*followingIndex)++], connections[i].endTile);
				}
				
				else if(strcmp("MUX", isMUX) == 0 && strcmp(connections[i].endTile, tile) != 0) {
					//printf("%s %s\n", connections[i].endTile, connections[i].endName);
					strcpy(LUTNames[*followingIndex], connections[i].endName);
					strcpy(LUTTiles[(*followingIndex)++], connections[i].endTile);
				}
				
				else if(connectionFound == 0) {
					connectionFound = 1;
					foundIndex = i;
				}
				else {
					pathBranches[branchIndex] = i;
					branchIndex++;
				}
			}
		}
		
		if(connectionFound == 0 && branchIndex > 0) {
			strcpy(searchTile, connections[pathBranches[--branchIndex]].endTile);
			strcpy(searchName, connections[pathBranches[branchIndex]].endName);
			connectionFound = 1;
		}
		else {
			strcpy(searchTile, connections[foundIndex].endTile);
			strcpy(searchName, connections[foundIndex].endName);
		}
		
	}
	
}

//A method to find all S-Boxes immediately following a given LUT ona given tile
void followingSBoxes(char tile[], char name[]) {
	
	
	char followingLUTNames[10][30];
	char followingLUTTiles[10][30];
	int followingIndex = 0;
	
	//Find the addRoundKey XOR LUTs following the given S-Box LUT
	followingLUTs(tile, name, followingLUTNames, followingLUTTiles, &followingIndex);
	
	char followingSBoxTiles[100][30];
	char followingSBoxNames[100][30];
	int followingSBoxIndex = 0;
	
	int followingFound = 0;
	int foundFollowerIndex = 0;
	
	char isLUT[5];
	
	
	//For each XOR LUT find the following S-Boxes
	for(int i = 0; i < followingIndex; i++) {
		
		followingFound = 0;
		
		printf("        %s %s ", followingLUTTiles[i], followingLUTNames[i]);
		
		followingLUTandMUX(followingLUTTiles[i], followingLUTNames[i], followingSBoxNames, followingSBoxTiles, &followingSBoxIndex);
		
		for(int j = 0; j < followingSBoxIndex; j++) {
			
			//printf("                  %s %s\n", followingSBoxTiles[j], followingSBoxNames[j]);
			
			for(int k = 0; k < sBoxIndex; k++) {
			
				for(int l = 0; l < 8; l++) {
					
					if(strcmp(finalSBoxes[k].LUT8s[l].name, followingSBoxTiles[j]) == 0) {
						
						if(finalSBoxes[k].LUT8s[l].LUT6s[0] == 1 && followingSBoxNames[j][0] < 69 && strlen(followingSBoxNames[j]) < 9) {
							followingSBoxIndexes[sBoxUnderExamination][foundFollowerIndex] = k;
							foundFollowerIndex++;
							followingFound = 1;
							printf("  [%d]\n", k);
							break;
						}
						else if(finalSBoxes[k].LUT8s[l].LUT6s[7] == 1 && followingSBoxNames[j][0] > 68 && strlen(followingSBoxNames[j]) < 9) {
							followingSBoxIndexes[sBoxUnderExamination][foundFollowerIndex] = k;
							foundFollowerIndex++;
							followingFound = 1;
							printf("  [%d]\n", k);
							break;
						}
						else if(finalSBoxes[k].LUT8s[l].LUT6s[0] == 1 && (strcmp("F8MUX_BOT_SEL", followingSBoxNames[j]) == 0 || strcmp("F7MUX_AB_SEL", followingSBoxNames[j]) == 0 || strcmp("F7MUX_CD_SEL", followingSBoxNames[j]) == 0)) {
							followingSBoxIndexes[sBoxUnderExamination][foundFollowerIndex] = k;
							foundFollowerIndex++;
							followingFound = 1;
							printf("  [%d]\n", k);
							break;
						}
						else if(finalSBoxes[k].LUT8s[l].LUT6s[7] == 1 && (strcmp("F8MUX_TOP_SEL", followingSBoxNames[j]) == 0 || strcmp("F7MUX_EF_SEL", followingSBoxNames[j]) == 0 || strcmp("F7MUX_GH_SEL", followingSBoxNames[j]) == 0)) {
							followingSBoxIndexes[sBoxUnderExamination][foundFollowerIndex] = k;
							foundFollowerIndex++;
							followingFound = 1;
							printf("  [%d]\n", k);
							break;
						}
					}	
				}
				
				if(followingFound == 1)
					break;
			
			}
			
			if(followingFound == 1)
				break;
			
		}
	}
	
	for(int i = 0; i < 5; i++) {
		printf(" %d, ", followingSBoxIndexes[sBoxUnderExamination][i]);
	}
	
	printf("\n");
	
	printf("------------------------------------\n");
	sBoxUnderExamination++;
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
	fputs("   - Byte: ", file);
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

int unusedLUTs(struct tile tile) {
	
	if(tile.LUT6s[0] == 2 && tile.LUT6s[1] == 2 && tile.LUT6s[2] == 2 && tile.LUT6s[3] == 2 
	&& tile.LUT6s[4] == 2 && tile.LUT6s[5] == 2 && tile.LUT6s[6] == 2 && tile.LUT6s[7] == 2) 
			return 1;
	else
		return 0;
}

//Detect the closest unused LUTs to the given Tile 
void closestUnusedLUTs(struct tile tile) {
	
	//The current tile being examined
	struct tile currentTile;
	
	struct tile closestTile[5];
	int closestIndex = 0;
	
	//Boolean indicating if an unused Tile has been found
	int LUTFound = 0;
	int newTile = 0;
	
	//If there are two Tiles at the same (x,y) check if the other is unused
	if(tilesSet[tile.x][tile.y][1] == 1) {
		if(strcmp(tile.name, (*cartTiles[tile.x][tile.y][1]).name) == 0) {
			currentTile = *cartTiles[tile.x][tile.y][0];
		}
		else {
			currentTile = *cartTiles[tile.x][tile.y][1];
		}
		
		if(unusedLUTs(currentTile) == 1) {
			
			closestIndex++;
			
			newTile = 1;
			
			for(int i = 0; i < unusedIndex; i++) {
				if(strcmp(unusedTiles[i].name, currentTile.name) == 0) {
					newTile = 0;
					unusedFrequency[i]++;
					break;
				}
			}
			
			if(newTile == 1) {
				unusedTiles[unusedIndex] = currentTile;
				unusedFrequency[unusedIndex]++;
				unusedIndex++;
			}
			
			if(closestIndex == 5)
				return;
		}
	}	

	//The current y Offset of tiles being examined
	int searchOffset = 1;
	int searching = 1;

	//Repeat until an unused Tile has been found
	while(searching == 1 && closestIndex < 5) {
		
		searching = 0;
		
		for(int i = -searchOffset; i < searchOffset + 1; i++) {
			for(int j = -searchOffset; j < searchOffset + 1; j++) {
				if(i != 0 || j != 0) {
					if(abs(tile.x + i) > searchOffset - 1 || abs(tile.y + j) > searchOffset - 1) {
						if(searchOffset < MAX_Y - tile.y && tile.y - searchOffset >= 0 && 
						   searchOffset < MAX_X - tile.x && tile.x - searchOffset >= 0) {
						  
							searching = 1;
							
							
							
							currentTile = *cartTiles[tile.x + i][tile.y + j][0];
							
							if(unusedLUTs(currentTile) == 1) {
								closestIndex++;
								//printf("          %s\n", currentTile.name);
								
								newTile = 1;
			
								for(int i = 0; i < unusedIndex; i++) {
									if(strcmp(unusedTiles[i].name, currentTile.name) == 0) {
										newTile = 0;
										unusedFrequency[i]++;
										break;
									}
								}
								
								if(newTile == 1) {
									unusedTiles[unusedIndex] = currentTile;
									unusedFrequency[unusedIndex]++;
									unusedIndex++;
								}
								
								if(closestIndex == 5)
									return;
								//printTile(currentTile);
								//return currentTile;
							}
						
							if(tilesSet[tile.x + i][tile.y + j][1] == 1) {
								currentTile = *cartTiles[tile.x + i][tile.y + j][1];
								
								if(unusedLUTs(currentTile) == 1) {
									closestIndex++;
									//printf("          %s\n", currentTile.name);
									
									newTile = 1;
			
									for(int i = 0; i < unusedIndex; i++) {
										if(strcmp(unusedTiles[i].name, currentTile.name) == 0) {
											newTile = 0;
											unusedFrequency[i]++;
											break;
										}
									}
									
									if(newTile == 1) {
										unusedTiles[unusedIndex] = currentTile;
										unusedFrequency[unusedIndex]++;
										unusedIndex++;
									}
			
									if(closestIndex == 5)
										return;
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

int main (int argc, char *argv[]) {	
	
	//Open the connection dictionaries and copy into the relevent dictionary
	char nextLine[30];
	char bramConnectionDictionary[200][3][30];
	
	FILE *bramDictionary = fopen("./bramConnections.txt", "r"); 
	FILE *logicDictionary = fopen("./formattedLogic.txt", "r"); 
	FILE *tileDictionary = fopen("./formattedTiles.txt", "r"); 
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
		/*
		getTileCoords(nextLine, &tileX, &tileY);
		printf("%d ", tileY % 5);
		printf(nextLine);
		*/
	}
	/*
	for(int i = 0; i < bramIndex; i++) {
		printf(bramConnectionDictionary[i][LOGIC]);
		printf(" %d\n", strlen(bramConnectionDictionary[i][LOGIC]));
	}
	return 0;
	
	
	//TESTING
	char bramTest[30] = "BRAM_BRAM_CORE_2_DOBL10";
	char bramNumber[3];
	int numberEncountered = 0;
	for(int i = 0; i < strlen(bramTest); i++) {
		if(bramTest[i] < 65) {
			if(numberEncountered != 0) {
				bramNumber[numberEncountered-1] = bramTest[i];
				numberEncountered++;
			}
			else 
				numberEncountered = 1;
		}
	}
	bramNumber[numberEncountered-1] = 0;
	printf("%d\n", atoi(bramNumber));
	
	return 0;
	*/
	
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
	
	//TESTING
	/*
	char logicTest[11];
	for(int i = 0; i < bramIndex; i++){
		for(int j = 0; j < connectionIndex; j++) {
			
			if(strcmp(connections[j].beginName, bramConnectionDictionary[i][LOGIC]) == 0) {
				if(strlen(connections[j].beginName) < 14)
					printConnection(connections[j]);
			}
			
			
		}
	}
	return 0;
	
	for(int i = 0; i < connectionIndex; i++) {
			
		for(int j = 0; j < 10; j++) {
			logicTest[j] = connections[i].beginName[j];
		}
		logicTest[10] = 0;
		
		if(strcmp(logicTest, "LOGIC_OUTS") == 0) {
			for(int j = 0; j < bramIndex; j++) {
				
			}
		}
	}
	*/
	
	
	//Open the file to write found S-Boxes to 
	FILE * foundThem = fopen("./FPGA_S-Boxes.txt", "w");

	//Initialise relevant variables and generate all LUT INIT values
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
					XORcount++;
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
	
	//Sort tiles into the S-Boxes to which they belong
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
	
	sBoxIndex--;
	
	//Process LUT8s with unfound roots
	struct tile unconnectedTile[10];
	int unconnectedIndex = 0;
	int incompleteSBox = 0;
	for (int i = 0; i < sBoxIndex; i++) {
		if(sBoxes[i].foundLUT8s < 8 && sBoxes[i].foundLUT8s > 0) {
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
	
	//TESTING
	printf("%d ", finalSBoxes[0].byte);
	finalSBoxes[0].byte = 10;
	printf("%d ", finalSBoxes[0].byte);
	

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
	
	//Go through rooting info to find each S-Boxes proceeding S-Box
	for(int sbIndex = 0; sbIndex < sBoxCount; sbIndex++) {
		afterSboxesTotal = 0;

		for(int lutIndex = 0; lutIndex < 8; lutIndex++) {
				retracing = 0;
				LUTfound = 0;
				sBoxFound = 0;
				branches = 0;
				choices = 0;

				if(finalSBoxes[sbIndex].LUT8s[lutIndex].LUT6s[0] == 1) {
					strcpy(searchName, "FFMUXC1_OUT1");
				}
				else {
					strcpy(searchName, "FFMUXG1_OUT1");
				}
				strcpy(searchTile, finalSBoxes[sbIndex].LUT8s[lutIndex].name);
				
				
				while(updated == 1) {
					updated = 0;
					
					for(int cIndex = 0; cIndex < connectionIndex; cIndex++) {
				
						if(strcmp(searchTile, connections[cIndex].beginTile) == 0 && strcmp(searchName, connections[cIndex].beginName) == 0) {
					
							
							
							updated = 1;
							strcpy(nextSearchTile, connections[cIndex].endTile);
							strcpy(nextSearchName, connections[cIndex].endName);

							if(testing == 1) {
								printf("%s %s %s\n", searchTile, searchName, connections[cIndex].endName);
							}
							
							for (int i = 0; i < 8; i++) {
								LUTtest[i] = searchName[i+1];
								if (i == 7) {
									LUTtest[i] = 0;
								}
							}
							
							
							
							for (int i = 0; i < 4; i++) {
								tileTest[i] = searchTile[i];
								if(i == 3) {
									tileTest[i] = 0;
								}
							}
							
							for(int i = 0; i < 5; i++) {
								MUXtest[i] = searchName[i+1];
								if(i == 4)
									MUXtest[i] = 0;
							}
							

							
							for(int i = strlen(searchName) - 3; i < strlen(searchName); i++) {
								MUXtest[7+i-strlen(searchName)] = searchName[i];
							}
							MUXtest[7] = 0;
							MUXnumber = searchName[1];
							
							if(strcmp("INT", tileTest) == 0 && retracing == 0) {
								strcpy(possibleRootTiles[branches][choices], connections[cIndex].endTile);
								strcpy(possibleRootNames[branches][choices], connections[cIndex].endName);
								choices++;
							}
							
							
							
							if((strcmp(LUTtest, "6LUT_O6") == 0 || strcmp(MUXtest, "7MUXSEL") == 0 || strcmp(MUXtest, "8MUXSEL") == 0) && strcmp(foundTile, searchTile) != 0) {
								LUTfound++;
								if(strcmp(LUTtest, "6LUT_O6") == 0)
									LUTletter = searchName[0];
								else {
									if(MUXnumber == 55)
										LUTletter = searchName[6];
									else {
										for(int i = 0; i < 4; i++) {
											MUXtest[i] = searchName[i+6];
											if(i == 3)
												MUXtest[i] = 0;
										}
										
										if(strcmp("TOP", MUXtest) == 0) 
											LUTletter = 70;
										else
											LUTletter = 66;
									}
								}

								//printf("%s %s %s\n", searchTile, searchName, connections[cIndex].endName);
								strcpy(foundTile, connections[cIndex].beginTile);

								for(int i = 0; i < sBoxIndex; i++) {
									for(int j = 0; j < 8; j++) {
										if(strcmp(finalSBoxes[i].LUT8s[j].name, foundTile) == 0) {

											if(finalSBoxes[i].LUT8s[j].LUT6s[7] == 0) {
												if(LUTletter < 69 && LUTletter > 64) {
													if(testing == 1) {
														printf("Following sBox found:\n");
														printSbox(finalSBoxes[i]);
													}
													finalSBoxes[i].previousSbox = &finalSBoxes[sbIndex];
													finalSBoxes[sbIndex].nextSbox = &finalSBoxes[i];
													sBoxFound = 1;
													afterSboxesTotal++;
													break;
												}
											}
											else if(LUTletter > 68 && LUTletter < 73) {
												if(testing == 1) {
													printf("Following sBox found:\n");
													printSbox(finalSBoxes[i]);
												}
												finalSBoxes[i].previousSbox = &finalSBoxes[sbIndex];
												finalSBoxes[sbIndex].nextSbox = &finalSBoxes[i];
												sBoxFound = 1;
												afterSboxesTotal++;
												break;
											}
											
											
										}
									}
									if(sBoxFound == 1)
										break;
								}
							}
							
						}
					}
					
					if(choices > 1 && retracing == 0) {
						choicesPerBranch[branches] = choices;
						branches++;
					}
					choices = 0;
					
					foundTile[0] = 0;
					strcpy(searchName, nextSearchName);
					strcpy(searchTile, nextSearchTile);
					
					if(sBoxFound == 1)
						break;
					
					//TODO test removal of sBoxFound == 0
					if(updated == 0 && sBoxFound == 0 && branches > 0) {
						
						if(retracing == 0) {
							branches--;
							retraceCounter = 0;
						}
						
						retracing = 1;

						strcpy(searchName, possibleRootNames[branches][retraceCounter]);
						strcpy(searchTile, possibleRootTiles[branches][retraceCounter]);
						
						retraceCounter++;
						updated = 1;
						if(retraceCounter == choicesPerBranch[branches] - 1) {
							branches--;
							retraceCounter = 0;
						}
						
						
						LUTfound = 1;
					}
					
					
					if(LUTfound > 1)
						break;
					
					if(testing == 1)
						printf("-----------------------------------------\n");
				}
			
				updated = 1;
		}
		
		finalSBoxes[sbIndex].followingLUTs = afterSboxesTotal;
		
		printf("\rProgress: %d / %d", sbIndex, tileIndex/8);
		fflush(stdout);
	}
	
	printf("\rProgress: %d / %d", tileIndex/8, tileIndex/8);
	fflush(stdout);
	
	int finalRoundSBoxes[16];
	int finalIndex = 0;
	int count = 0;
	/*
	FILE * roundSBoxes = fopen("sortedS-Boxes.txt", "w");
	fputs("FINAL ROUND:", roundSBoxes);
	fputc(10, roundSBoxes);
	fputs("----------------------", roundSBoxes);
	fputc(10, roundSBoxes);
	*/
	
	//If a S-Box has no following S-Boxes then it must belong in the final round
	//TODO: for ever final roudn S-Box LUT8 find the closest unused LUT4 and detnote 
	for(int i = 0; i < sBoxIndex; i++) {
		if(finalSBoxes[i].followingLUTs == 0 && finalSBoxes[i].previousSbox != NULL) {
			finalRoundSBoxes[finalIndex] = i;
			finalIndex++;
			//fputSbox(finalSBoxes[i], i, roundSBoxes);
			finalSBoxes[i].round = 10;
			count++;
		}
	}
	
	printf("\nS-Boxes per round: %d\n", count);
	
	//If an S-Box is followed by a round 10 S-Box it must be in round 9 and so on 
	for(int rounds = 9; rounds > 0; rounds --) {
		/*
		fputs("ROUND ", roundSBoxes);
		fputc(rounds + 48, roundSBoxes);
		fputc(10, roundSBoxes);
		fputs("----------------------", roundSBoxes);
		fputc(10, roundSBoxes);
		*/
		for(int i = 0; i < sBoxIndex; i++) {
	
			if(finalSBoxes[i].nextSbox != NULL) {
				if(finalSBoxes[i].nextSbox->round == rounds + 1) {
					finalSBoxes[i].round = rounds;
					//fputSbox(finalSBoxes[i], i, roundSBoxes);
				}
			}
		}	
	}	
	
	count = 0;
	
	//Any remaining S-Boxes must be used for key generation
	/*
	fputs("KEY S-BOXES:", roundSBoxes);
	fputc(10, roundSBoxes);
	fputs("----------------------", roundSBoxes);
	*/
	for(int i = 0; i < sBoxCount; i++) {
		if(finalSBoxes[i].nextSbox == NULL && finalSBoxes[i].previousSbox == NULL) {
			//fputSbox(finalSBoxes[i], i, roundSBoxes);
			count++;
		}
	}
	
	
	printf("S-Boxes used in key generation: %d\n", count);
	
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
	int testingTesting = 0;
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
	int groupSet = 0;
	int oneIndex = 0;
	int twoIndex = 0;
	int threeIndex = 0;
	int fourIndex = 0;
	double yAverages[4][4];
	double yAverage = 0;
	double foundCount = 0;
	
	//Go backwards from round 1 S-Boxes until the BRAM is found
	for(int i = 0; i < sBoxIndex; i++) {
		
		if(finalSBoxes[i].round == 1) {
			
			groupSet = 0;
			yAverage = 0;
			foundCount = 0;
			
			for(int j = 0; j < 8; j++) {
				
				searching = 0;
				branchIndex = 0;
				tilesFound = 0;
				
				if((&finalSBoxes[i])->LUT8s[j].bitA != 10) {
					strcpy(searchTile, (&finalSBoxes[i])->LUT8s[j].name);
					strcpy(searchName, "A6LUT_O6");
					searching =1;
					//TESTING
					//printTile((&finalSBoxes[i])->LUT8s[j]);
				}
				else if((&finalSBoxes[i])->LUT8s[j].bitE != 10) {
					strcpy(searchTile, (&finalSBoxes[i])->LUT8s[j].name);
					strcpy(searchName, "E6LUT_O6");
					searching = 1;
					//TESTING
					//printTile((&finalSBoxes[i])->LUT8s[j]);
				}
				
				if(j == 0)
					printTile((&finalSBoxes[i])->LUT8s[j]);
				
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
								//printConnection(connections[k]);
								
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
								
								//TESTING
								//printf("      ");
								//printConnection(connections[k]);
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
						
						if(strcmp(testString, "INT_X8") == 0 && tilesFound <= 2) {
							//printConnection(connections[finalConnection]);
							searching = 0;
							break;
						}
						if(branchIndex > 0) {
							
							//TESTING
							//printf("----BRANCHING----\n");
							
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
							printf(bramConnectionDictionary[k][BRAM]);
							
							foundCount++;
							yAverage += tempY;
							
							break;
						}
					}
				}
				
				if(j == 7) {
					if(atoi(bramNumber) < 3) {
						wordGroups[0][oneIndex] = i;
						yAverages[0][oneIndex++] = yAverage/foundCount;
					} else if(atoi(bramNumber) < 7) {
						wordGroups[1][twoIndex] = i;
						yAverages[1][twoIndex++] = yAverage/foundCount;
					} else if(atoi(bramNumber) < 11) {
						wordGroups[2][threeIndex] = i;
						yAverages[2][threeIndex++] = yAverage/foundCount;
					} else {
						wordGroups[3][fourIndex] = i;
						yAverages[3][fourIndex++] = yAverage/foundCount;
					}
				}				
				

				//printf("----------------------------------------\n");
				
			}
			printf("-----------------------------------\n");
			
		}
	}
	
	for(int i = 0; i < 4; i++) {
		printf("GROUP %d: ", i+1);
		for(int j = 0; j < 4; j++) {
			printf("%d [%f], ", wordGroups[i][j], yAverages[i][j]);
			//printSbox(finalSBoxes[wordGroups[i][j]]);
		}
		printf("\n");
	}
	
	int followingLUTindex = 0;
	int connectionFound = 1;
	char originalLUTs[16][30];
	
	int associatedIndex[16];

	for(int i = 0; i < sBoxIndex; i++) {
	
		if(finalSBoxes[i].round == 1) {
		
			for(int j = 0; j < 8; j++) {
			
				if((&finalSBoxes[i])->LUT8s[j].bitA == 1) {
					printTile((&finalSBoxes[i])->LUT8s[j]);
					strcpy(originalLUTs[sBoxUnderExamination], (&finalSBoxes[i])->LUT8s[j].name);
					associatedIndex[sBoxUnderExamination] = i;
					followingSBoxes((&finalSBoxes[i])->LUT8s[j].name, "A6LUT_O6");
				}
				else if((&finalSBoxes[i])->LUT8s[j].bitE == 1) {
					printTile((&finalSBoxes[i])->LUT8s[j]);
					strcpy(originalLUTs[sBoxUnderExamination], (&finalSBoxes[i])->LUT8s[j].name);
					associatedIndex[sBoxUnderExamination] = i;
					followingSBoxes((&finalSBoxes[i])->LUT8s[j].name, "E6LUT_O6");
				}
			}
		}
	}
	
	for(int i = 0; i < 5; i++) {
		printf(" %d, ", followingSBoxIndexes[sBoxUnderExamination][i]);
	}
	
	for(int i = 0; i < 16; i++) {
		for(int j = 0; j < 5; j++) {
			for(int k = 0; k < 5; k++) {
			
				if(followingSBoxIndexes[i][j] == followingSBoxIndexes[i][k] && j != k) {
					finalSBoxes[associatedIndex[i]].nextIndex = followingSBoxIndexes[i][j];
				}
			
			}
		}
	}
	
	int columns[4][5];
	int columnMembership[16];
	int columnIndex = 0;
	int row0, row1, row2, row3 = 0;
	
	
	int existingColumn = 0;
	int foundIndex = 0;
	
	printf("\n --------------------------------------------\n");
	
	//For all S-Boxes in the round
	for(int i = 0; i < 16; i++) {
	
		existingColumn = 0;
	
		//For all followingSBoxIndexes
		for(int j = 0; j < 5; j++) {
		
			for(int k = 0; k < columnIndex; k++) {
		
				if(followingSBoxIndexes[i][0] == columns[k][j]) {
					existingColumn = 1;
					foundIndex = k;
				}
				
			}
		}
		
		if(existingColumn == 0) {
			for(int j = 0; j < 5; j++) {
				columns[columnIndex][j] = followingSBoxIndexes[i][j];
			}
			columnMembership[i] = columnIndex;
			finalSBoxes[associatedIndex[i]].column = columnIndex;
			columnIndex++;
		}
		else {
			columnMembership[i] = foundIndex;
			finalSBoxes[associatedIndex[i]].column = foundIndex;
		}
	}
	
	for(int i = 0; i < 16; i++) {
		printf("%d [%d]\n", associatedIndex[i], finalSBoxes[associatedIndex[i]].column);
	}
	
	int bytes[4][4];
	int isLowerHalf[4][4];
	
	int lessThan = 0;
	
	for(int i = 0; i < 4; i++) {

		for(int j = 0; j < 4; j++) {
			
			lessThan = 0;
			
			
			
			for(int k = 0; k < 4; k++) {
				if(j != k) {
					if(yAverages[i][j] < yAverages[i][k])
						lessThan++;
				}
			}

			
			if(lessThan >= 2)
				isLowerHalf[i][j] = 0;
			else
				isLowerHalf[i][j] = 1;
		}

	}
	
	
	
	int sameColumnLower = 0;
	int sameColumnUpper = 0;
	
	for(int groupIndex = 0; groupIndex < 3; groupIndex++) {
		
		sameColumnLower = 0;
		sameColumnUpper = 0;
		
		for(int i = 0; i < 4; i++) {
			if(isLowerHalf[groupIndex][i] == 1){
				if(sameColumnLower == 1)
					bytes[groupIndex][i] = groupIndex + 4;
				else {
					for(int j = 0; j < 4; j++) {
						if(isLowerHalf[groupIndex+1][j] == 1){
							if(finalSBoxes[wordGroups[groupIndex][i]].column == finalSBoxes[wordGroups[groupIndex+1][j]].column ) {
								bytes[groupIndex][i] = groupIndex; //0;
								sameColumnLower = 1;
							}
						}
					}
					if(sameColumnLower == 0)
						bytes[groupIndex][i] = groupIndex + 4; //4;
				}
			}
			else {
				if(sameColumnUpper == 1)
					bytes[groupIndex][i] = groupIndex + 12;
				else {
					for(int j = 0; j < 4; j++) {
						if(isLowerHalf[groupIndex+1][j] == 0){
							if(finalSBoxes[wordGroups[groupIndex][i]].column == finalSBoxes[wordGroups[groupIndex+1][j]].column ) {
								bytes[groupIndex][i] = groupIndex + 8;
								sameColumnUpper = 1;
							}
						}
					}
					if(sameColumnUpper == 0)
						bytes[groupIndex][i] = groupIndex + 12;
				}
			}
		}
	}
	
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			if(finalSBoxes[wordGroups[3][i]].column == finalSBoxes[wordGroups[0][j]].column) {
				switch(bytes[0][j]) {
				
					case 0:
						bytes[3][i] = 15;
						break;
					case 4:
						bytes[3][i] = 3;
						break;
					case 8:
						bytes[3][i] = 7;
						break;
					case 12:
						bytes[3][i] = 11;
						break;
					
						
				}
			}
		}
	}
	
	for(int i = 0; i < 4; i++) {
		printf("GROUP %d: ", i+1); 
		for(int j = 0; j < 4; j++) {
			printf("%d [%d], ", wordGroups[i][j], bytes[i][j]);
			finalSBoxes[wordGroups[i][j]].byte = bytes[i][j];
		}
		printf("\n");
	}
	
	struct sBox * tempSBox;
	int followingTestCount = 0;
	
	sBoxUnderExamination = 0;
	int tempIndexes[200];
	
	for(int round = 2; round < 5; round ++) {
		
		sBoxUnderExamination = 0;
		
		for(int i = 0; i < sBoxIndex; i++) {
		
			if(finalSBoxes[i].round == round) {
			
				for(int j = 0; j < 8; j++) {
				
					if((&finalSBoxes[i])->LUT8s[j].bitA == 1) {
						//printTile((&finalSBoxes[i])->LUT8s[j]);
						//strcpy(originalLUTs[sBoxUnderExamination], (&finalSBoxes[i])->LUT8s[j].name);
						tempIndexes[sBoxUnderExamination] = i;
						followingSBoxes((&finalSBoxes[i])->LUT8s[j].name, "A6LUT_O6");
					}
					else if((&finalSBoxes[i])->LUT8s[j].bitE == 1) {
						//printTile((&finalSBoxes[i])->LUT8s[j]);
						//strcpy(originalLUTs[sBoxUnderExamination], (&finalSBoxes[i])->LUT8s[j].name);
						tempIndexes[sBoxUnderExamination] = i;
						followingSBoxes((&finalSBoxes[i])->LUT8s[j].name, "E6LUT_O6");
					}
				}
			}
		}
		
		for(int i = 0; i < sBoxUnderExamination; i++) {
			for(int j = 0; j < 5; j++) {
				for(int k = 0; k < 5; k++) {
				
					if(followingSBoxIndexes[i][j] == followingSBoxIndexes[i][k] && j != k) {
						finalSBoxes[tempIndexes[i]].nextIndex = followingSBoxIndexes[i][j];
					}
				
				}
			}
		}
	}
	
	for(int i = 0; i < 16; i++) {
		
		tempSBox = &finalSBoxes[associatedIndex[i]];
		printf("%d:\n", associatedIndex[i]);
		followingTestCount = 0;
		
		while(tempSBox->nextIndex > 0) {
			
			followingTestCount++;
			printf("	%s\n", (&finalSBoxes[tempSBox->nextIndex])->LUT8s[0].name);
			
			switch(tempSBox->byte) {
				
				case 0:
					finalSBoxes[tempSBox->nextIndex].byte = 13;
					break;
					
				case 1:
					finalSBoxes[tempSBox->nextIndex].byte = 10;
					break;
					
				case 2: 
					finalSBoxes[tempSBox->nextIndex].byte = 7;
					break;
					
				case 3:
					finalSBoxes[tempSBox->nextIndex].byte = 0;
					break;
					
				case 4:
					finalSBoxes[tempSBox->nextIndex].byte = 1;
					break;
					
				case 5:
					finalSBoxes[tempSBox->nextIndex].byte = 14;
					break;
					
				case 6:
					finalSBoxes[tempSBox->nextIndex].byte = 11;
					break;
					
				case 7:
					finalSBoxes[tempSBox->nextIndex].byte = 4;
					break;
					
				case 8:
					finalSBoxes[tempSBox->nextIndex].byte = 5;
					break;
					
				case 9:
					finalSBoxes[tempSBox->nextIndex].byte = 2;
					break;
					
				case 10:
					finalSBoxes[tempSBox->nextIndex].byte = 15;
					break;
					
				case 11:
					finalSBoxes[tempSBox->nextIndex].byte = 8;
					break;
					
				case 12:
					finalSBoxes[tempSBox->nextIndex].byte = 9;
					break;
					
				case 13:
					finalSBoxes[tempSBox->nextIndex].byte = 6;
					break;
					
				case 14:
					finalSBoxes[tempSBox->nextIndex].byte = 3;
					break;
					
				case 15:
					finalSBoxes[tempSBox->nextIndex].byte = 12;
					break;
					
			
			}
			
			tempSBox = &finalSBoxes[tempSBox->nextIndex];
		}
		printf("%d\n", followingTestCount);
		
	}
	
	FILE * roundSBoxes = fopen("sortedS-Boxes.txt", "w");
	fputs("FINAL ROUND:", roundSBoxes);
	fputc(10, roundSBoxes);
	fputs("----------------------", roundSBoxes);
	fputc(10, roundSBoxes);
	
	for(int i = 0; i < sBoxIndex; i++) {
		if(finalSBoxes[i].round == 10)
			fputSbox(finalSBoxes[i], i, roundSBoxes);
	}
	
	//If an S-Box is followed by a round 10 S-Box it must be in round 9 and so on 
	for(int rounds = 9; rounds > 0; rounds --) {
	
		fputs("ROUND ", roundSBoxes);
		fputc(rounds + 48, roundSBoxes);
		fputc(10, roundSBoxes);
		fputs("----------------------", roundSBoxes);
		fputc(10, roundSBoxes);
		for(int i = 0; i < sBoxIndex; i++) {
	
			if(finalSBoxes[i].round == rounds) 
				fputSbox(finalSBoxes[i], i, roundSBoxes);
			
		}	
	}
	
	//Any remaining S-Boxes must be used for key generation
	fputs("KEY S-BOXES:", roundSBoxes);
	fputc(10, roundSBoxes);
	fputs("----------------------", roundSBoxes);
	for(int i = 0; i < sBoxCount; i++) {
		if(finalSBoxes[i].nextSbox == NULL && finalSBoxes[i].previousSbox == NULL) {
			fputSbox(finalSBoxes[i], i, roundSBoxes);
			count++;
		}
	}
}