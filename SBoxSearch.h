
FILE *RouteData;
char next;
char word[65];
int wordIndex = 0;

//Retrive the next word from the json file
void getNextWord(char word[]) {

	do {
		next = fgetc(RouteData);
	} while (next != 34 && next != EOF);
	
	do {
		next = fgetc(RouteData);
		word[wordIndex] = next;
		wordIndex++;
	} while (next != 34 && next != EOF);
			
	word[wordIndex - 1] = 0;
	wordIndex = 0;
}

//A data structure to represent a wire connection in the FPGA and an array
struct connection{
	char beginName[30];
	char beginTile[35];
	char endName[30];
	char endTile[35];
};


	//Create and return a new connection 
	struct connection newConnection() {
		struct connection newConnection;
		return newConnection;
	}

	//Print a connection to the console
	void printConnection(struct connection connection) {
		printf("%26-s (%13-s)", connection.beginName, connection.beginTile);
		printf("   -------->   %s (%s)\n", connection.endName, connection.endTile);
	}

	//Read the connection name and tile into the given string locations
	void setTileAndName(char tile[], char name[]) {
		
		//Set the name
		do {
			getNextWord(word);
		} while (strcmp(word, "name") != 0);
		getNextWord(name);
				
		//Set the  tile name
		do {
			getNextWord(word);
		} while (strcmp(word, "name") != 0);
		getNextWord(tile);
				
		//Set the tile x-coordinate
		int length = strlen(tile);
		int tileOffset = 2;
		tile[length] = 95;
		tile[length+1] = 88;
		getNextWord(word);
		next = fgetc(RouteData);
		while(next != 44) {
			next = fgetc(RouteData);
			tile[length + tileOffset] = next;
			tileOffset++;
		}
				
		//Set the tile y-coordinate
		tileOffset--;
		tile[length + tileOffset] = 89;
		getNextWord(word);
		next = fgetc(RouteData);
		while(next != 125) {
			next = fgetc(RouteData);
			tileOffset++;
			tile[length + tileOffset] = next;
		}
		tile[length + tileOffset] = 0;
	}

//A data structure representitive of an FPGA tile, the encoding of LUTs is:
//1 corresponds to the LUT being set, e.g. 11110000 means A-D are set
struct tile{
	char name[30];
	int x,y; 
	int LUT6s[8]; 
	int bitA;
	int bitE;
};

	//Create and return a new tile with the given name
	struct tile createTile(char name[], int x, int y) {
		struct tile newTile;
		strcpy(newTile.name, name);
		newTile.x = x;
		newTile.y = y;
		for (int i = 0; i < 8; i++) {
			newTile.LUT6s[i] = 0;
		}
		newTile.bitA = 10;
		newTile.bitE = 10;
		return newTile;
	}
	
	//Return a copy of the provided tile
	struct tile copyTile (struct tile original) {
		struct tile copy;
		strcpy(copy.name, original.name);
		copy.x = original.x;
		copy.y = original.y; 
		for (int i = 0; i < 8; i++) {
			copy.LUT6s[i] = original.LUT6s[i];
		}
		copy.bitA = original.bitA;
		copy.bitE = original.bitE;
		return copy;
	}

	//Print the tile passed as a parameter
	void printTile (struct tile tile) {
		printf("%s ", tile.name);
		for (int i = 0; i < 8; i ++) {
			printf("%d", tile.LUT6s[i]);
		}
		printf("  [%d] [%d]", tile.bitA, tile.bitE);
		printf("\n");
	}

//Data structure representing an SBox
struct sBox{
	char rootNames[6][30];
	char rootTiles[6][30];
	int foundRoots;
	struct tile LUT8s[8];
	int foundLUT8s;
	int followingLUTs;
	struct sBox * previousSbox;
	struct sBox * nextSbox;
	int round;
	int column;
};

	//Create and return a new sBox
	struct sBox newSBox(struct tile LUT8, char rootTiles[][30], char rootNames[][30], int foundRoots) {
		struct sBox sBox;
		sBox.foundRoots = foundRoots;
		sBox.foundLUT8s = 1;
		
		for (int i = 0; i < foundRoots; i++) {
			strcpy(sBox.rootTiles[i], rootTiles[i]);
			strcpy(sBox.rootNames[i], rootNames[i]);
		}
		
		sBox.LUT8s[0] = copyTile(LUT8);
		sBox.followingLUTs = 0;
		sBox.previousSbox = NULL;
		sBox.nextSbox = NULL;
		sBox.round = 0;
		sBox.column = -1;
		return sBox;
	}
	
	//Print the SBox passed as a parameter
	void printSbox(struct sBox sBox) {
	
		//Print the number of found SBox roots
		printf("Found Roots: %d\n", sBox.foundRoots);
		
		for(int i = 0; i < sBox.foundRoots; i++) {
			printf("%s\n", sBox.rootNames[i]);
			printf("%s\n", sBox.rootTiles[i]);
		}

		//print the number of LUT8s found
		printf("Found LUT8s: %d\n", sBox.foundLUT8s);
		
		for (int i = 0; i < sBox.foundLUT8s; i++) {
			printTile(sBox.LUT8s[i]);
		}
		printf("--------------------------\n");
	}
	
	//Add a tile to the given sBox
	void addTile(struct sBox * sBox, struct tile tile) {
		sBox->LUT8s[sBox->foundLUT8s] = copyTile(tile);
		sBox->foundLUT8s++;
	}