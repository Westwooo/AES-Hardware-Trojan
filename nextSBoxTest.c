#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./SBoxSearch.h"

#define BRANCHES 10

#define X 88

#define BRAM 0
#define LOGIC 1
#define TILE 2

struct connection connections[1000000];
int connectionIndex = 0;

int testing = 0;
int lutFound = 0;

char lutTiles[10][30];
char foundTile[20];
char initialTile[30];
int foundIndex = 0;

int foundLUTs = 0;

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

int nextLUT(char tile[], char name[], int depth, int lutCount) {
		
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
					lutTest[i] = connections[cIndex].endName[i + 2];
				}
				lutTest[3] = 0;
				
				for(int i = 0; i < 3; i++) {
					muxTest[i] = connections[cIndex].endName[i+2];
				}
				muxTest[3] = 0;
				//printf(muxTest);
				
				if(strcmp(lutTest, "LUT") == 0 || strcmp(muxTest, "MUX") == 0 && strcmp(initialTile, connections[cIndex].endTile) != 0) {
					
					printf("%s (%s)\n", connections[cIndex].endName, connections[cIndex].endTile);
					
					if(testing == 1)
						printConnection(connections[cIndex]);
					
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
	
	if(depth > 0) {
		for(int i = branches - 1; i >= 0; i--) {
			for(int j = 0; j < 5; j++) {
				if(branchPoints[i][j] != NULL) {
					
					if(testing == 1) {
						printf("				-----BRANCH DEPTH %d-----\n", depth);
						printConnection(*branchPoints[i][j]);
						printf("--------------------------------------------------------\n");
					}
					lutFound = nextLUT(branchPoints[i][j]->endTile, branchPoints[i][j]->endName, depth, lutCount);
				}
			}
		}
	}
	
	return lutFound;
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
	
	
	struct connection lutConnection;
	
	//TO DO - this will set the following S-Box in the full tool 
	//nextLUT("CLEM_X13Y164", "FFMUXG1_OUT1", 1, 0);
	//nextLUT("CLEM_X13Y160", "A6LUT_O6", 1, 0);
	//nextLUT("CLEM_X18Y103", "FFMUXG1_OUT1", 1, 0);
	//strcpy(initialTile, "CLEM_X6Y162");
	//nextLUT("CLEM_X6Y162", "A6LUT_O6", 1, 0);
	//printf("%d\n", foundLUTs);
	
	//char searchTiles[16][30] = {"CLEL_L_X5Y156", "CLEL_L_X5Y157", "CLEL_L_X5Y162", "CLEL_R_X4Y156", "CLEL_R_X4Y159", "CLEL_R_X4Y162", "CLEL_R_X4Y163", "CLEL_R_X5Y156"};
	char searchTiles[16][30] = {"CLEL_L_X5Y156", "CLEL_L_X5Y160", "CLEL_L_X5Y169", "CLEL_L_X5Y174", 
								"CLEL_L_X5Y176", "CLEL_L_X7Y167", "CLEL_L_X7Y171", "CLEL_L_X7Y174", 
								"CLEL_R_X10Y174", "CLEL_R_X10Y177", "CLEL_R_X2Y158", "CLEL_R_X2Y162",
								"CLEL_R_X4Y169", "CLEL_R_X4Y172", "CLEL_R_X6Y169", "CLEL_R_X6Y171"};
	char searchNames[16][30] = {"CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_A1", 
								"CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_E1", "CLE_CLE_L_SITE_0_E1",
								"CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_A1",
								"CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_A1", "CLE_CLE_L_SITE_0_E1"};
	//CLE_CLE_M_SITE_0_A5, || CLE_CLE_L_SITE_0_A5
	
	char searchTile[30] = "CLEL_R_X2Y158";
	char searchName[30];
	char testSearchName[30];
	char testSearchTile[30];
	char lastTile[20];
	char testString[20];
	char tileTest[4];
	
	int tilesFound = 0;
	
	//Arays to handle branches in the FPGA paths
	int pathBranches[200];
	int branchPoint[200];
	int branchIndex = 0;

	int searching = 1;
	int finalConnection = 0;
	int groupSet = 0;
	int foundCount = 0;
	testing = 1;
	
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
	}
	/*
	for(int i = 0; i < bramIndex; i++) {
		printf("%s - %s - %s\n", bramConnectionDictionary[i][BRAM], bramConnectionDictionary[i][LOGIC], bramConnectionDictionary[i][TILE]);
	}
	*/
	
	int wordGroups[4][4];
	int lowerIndexes[4] = {0, 0, 0, 0};
	int upperIndexes[4] = {2, 2, 2, 2};
	int oneLowerIndex, oneUpperIndex = 0;
	int twoLowerIndex, twoUpperIndex = 0;
	int threeLowerIndex, threeUpperIndex = 0;
	int fourLowerIndex, fourUpperIndex = 0;
	int bytes[4][4];
	
	for(int i = 0; i < 4; i ++) {
		for(int j = 0; j < 4; j++) {
			bytes[i][j] = -1;
		}
	}
	
	double yAverage = 0;
	
	int bramFound = 0;
	int tempX, tempY;
	
	int followingSBoxes[16][5];
	
	for(int i = 0; i < 5; i++) {
		followingSBoxes[0][i] = i;
		followingSBoxes[1][i] = i;
		followingSBoxes[10][i] = i;
		followingSBoxes[11][i] = i;
		
		followingSBoxes[6][i] = i + 6;
		followingSBoxes[15][i] = i + 6;
		followingSBoxes[13][i] = i + 6;
		followingSBoxes[12][i] = i + 6;
		
		followingSBoxes[9][i] = i + 12;
		followingSBoxes[8][i] = i + 12;
		followingSBoxes[3][i] = i + 12;
		followingSBoxes[4][i] = i + 12;
		
		followingSBoxes[7][i] = i+ 18;
		followingSBoxes[5][i] = i + 18;
		followingSBoxes[2][i] = i + 18;
		followingSBoxes[14][i] = i + 18;
	}
	

	//For each round one S-Box
	for(int i = 0; i < 16; i++) {
		
		printf(searchTiles[i]);
		printf("\n");
		
		//For all 6 LUT inputs
		for(int j = 0; j < 6; j++) {
			
			searching = 1;
			groupSet = 0;
			yAverage = 0;
			foundCount = 0;
			branchIndex = 0;
			tilesFound = 0;
			
			
			/*
			if(searchTile[3] == 77)
				strcpy(searchName, "CLE_CLE_M_SITE_0_A1");
			else
				strcpy(searchName, "CLE_CLE_L_SITE_0_A1");
			*/
			//Initialise search name and tile
			strcpy(searchTile, searchTiles[i]);
			strcpy(searchName, searchNames[i]);
			searchName[strlen(searchName)-1] = searchName[strlen(searchName)-1] + j;
			
			//TESTING
			printf(searchName);
			printf("\n");
			
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
							if(testing == 1)
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
							
							//TESTING
							if(testing == 1){
								printf("      ");
								printConnection(connections[k]);
							}
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
						printConnection(connections[finalConnection]);
						searching = 0;
						break;
					}
					if(branchIndex > 0) {
						
						//TESTING
						if(testing == 1)
							printf("----BRANCHING----\n");
						
						strcpy(searchTile, connections[pathBranches[--branchIndex]].beginTile);
						strcpy(searchName, connections[pathBranches[branchIndex]].beginName);
						tilesFound--;
						searching = 1;
					}
				}
				
			}
			
			
			
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
						
						//TESTING
						printf("      ");
						printf(bramConnectionDictionary[k][BRAM]);
						printf("%d\n", atoi(bramNumber));
						
						j = 5;
						
						foundCount++;
						yAverage += tempY;
						
						break;
					}
				}
			}
			
			if(j == 5) {
				
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
				
				
				printf("%d\n", tempX);
			}
			
		}
		printf("---------------------------------\n");
	}
	
	int isZero = 0;
	
	for(int i = 0; i < 5; i++) {
		if(followingSBoxes[wordGroups[0][0]][0] == followingSBoxes[wordGroups[1][0]][i] || followingSBoxes[wordGroups[0][0]][0] == followingSBoxes[wordGroups[1][1]][i]) {
			printf("HERE\n");
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
					if(followingSBoxes[wordGroups[0][idIndex]][0] == followingSBoxes[wordGroups[i][j]][k]) {
					
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
		if(followingSBoxes[wordGroups[0][2]][0] == followingSBoxes[wordGroups[1][2]][i] || followingSBoxes[wordGroups[0][2]][0] == followingSBoxes[wordGroups[1][3]][i]) {
			printf("HERE\n");
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
					if(followingSBoxes[wordGroups[0][idIndex]][0] == followingSBoxes[wordGroups[i][j]][k]) {
					
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
		printf("GROUP %d: ", i+1);
		for(int j = 0; j < 4; j++) {
			printf("%d [%d], ", wordGroups[i][j], bytes[i][j]);
		}
		printf("\n");
	}
	
}