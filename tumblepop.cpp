#include <iostream>
#include <fstream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <ctime>
#include <cstdlib>

using namespace sf;
using namespace std;

int screen_x = 1152;
int screen_y = 896;
void jump(bool& onGround, float& velocityY, const float jumpStrength);
void display_level(RenderWindow& window, char**lvl, Texture& bgTex,Sprite& bgSprite,Texture& blockTexture,Sprite& blockSprite, const int height, const int width, const int cell_size)
{
	window.draw(bgSprite);

	for (int i = 0; i < height; i += 1)
	{
		for (int j = 0; j < width; j += 1)
		{

			if (lvl[i][j] == '#' || lvl[i][j] == '!')
			{
				blockSprite.setPosition(j * cell_size, i * cell_size);
				window.draw(blockSprite);
			}
		}
	}
}

			//BOUNDARY FORMATION
void boundaries(char**lvl, const int height, const int width){
	for (int i = 0; i < height; i++)
	{
		for (int j=0;  j< width; j++)
		{
			if (i==0 || j==0 || i==height-1 || j==width-1)
				lvl[i][j] = '#';
			else
				lvl[i][j] = ' ';
		}	
	}
}

void initGhosts(float ghostX[], float ghostY[], int ghostDir[], float ghostSpeed[], int ghost_count, int ghostTimer[])
{
    // rows where ghosts can safely stand
    int spawnRows[4] = {2, 5, 9, 12};  // above platforms at 3, 6, 10 and ground
    int rowCount = 4;

    int colStart = 4;     // starting column
    int colStep  = 10;     // spacing between ghosts

    int g = 0;

    // place two ghosts per row until we fill all 8
    for(int r = 0; r < rowCount && g < ghost_count; r++)
    {
        for(int c = 0; c < 2 && g < ghost_count; c++)
        {
            ghostX[g] = (colStart + c * colStep)*64;
            ghostY[g] = (spawnRows[r]*64)-32;             // different rows
            ghostDir[g] = 1;                      // means ghost should move right
            ghostSpeed[g] = 1.2f;
            g++;
        }
    }
}

void moveGhosts(char** lvl, int height, int width, float ghostX[], float ghostY[], int ghostDir[],float ghostMoveSpeed[],int ghost_count, char ghostFace[])
{
    const int cell_size = 64;
	const int ghostWidth = 64;

    for(int g = 0; g < ghost_count; g++)
    {

		if(ghostX[g] == 0 && ghostY[g] == 0) // If ghost at this index number is captured, skipping iteration
        continue;

        int ghostPositionX = (int)(ghostX[g] / cell_size);
        int ghostPositionY = (int)(ghostY[g] / cell_size);

        float predictedX = ghostX[g] + (ghostMoveSpeed[g] * ghostDir[g]);
        int nextTileX = (int)(predictedX / cell_size)+32;

        // Wall hit check
		if (ghostDir[g] == -1)
		{
			ghostFace[g] = 'L';
			if(lvl[ghostPositionY][nextTileX] == '#')
			{
				ghostDir[g] *= -1;
				ghostFace[g] = 'R';
				continue;
			}
		}
		else
		{
			ghostFace[g] = 'R';
			if(lvl[ghostPositionY][nextTileX + 1] == '#')
			{
				ghostDir[g] *= -1;
				ghostFace[g] = 'L';
				continue;
			}
		}

        // Floor hit check
		if(ghostDir[g] == 1)
		{
			ghostFace[g] = 'R';
			if(lvl[ghostPositionY + 1][nextTileX + 1] != '#')
			{
				ghostDir[g] *= -1;
				ghostFace[g] = 'L';
				continue;
			}
		}
		else
		{
			ghostFace[g] = 'L';
			if(lvl[ghostPositionY + 1][nextTileX] != '#')
			{
				ghostDir[g] *= -1;
				ghostFace[g] = 'R';
				continue;
			}
		}
		// Movement on platform
        ghostX[g] += ghostMoveSpeed[g] * ghostDir[g];
    }
}

void initSkeletons(float skeleton_x[], float skeleton_y[],float skeletonDir[], float skeletonSpeed[],int skeleton_count, const int cell_size,int skeletonState[],int skeletonCooldown[],bool skeletonOnGround[], float skeletonY_velocity[])
{
    int spawnRows[4] = { 2, 5, 9, 12 }; 

    for (int i = 0; i < skeleton_count; i++)
    {
        skeleton_x[i] = 5 * cell_size;                // same column num for spawning of all skeltons (column 5)
        skeleton_y[i] = (spawnRows[i] * cell_size) - 32;

        skeletonDir[i] = 1;          // facing right
        skeletonSpeed[i] = 1.0f; // movement speed of skeletons
		
		skeletonState[i] = 0;
		skeletonCooldown[i] = rand() % 61 + 30;   // 0.5–1.5 seconds before changing state

		skeletonOnGround[i] = true;
        skeletonY_velocity[i]     = 0.0f;
	}

}

void moveSkeletons(char** lvl, int height, int width,float skeleton_x[], float skeleton_y[],float skeletonDir[], float skeletonSpeed[],int skeletonState[], int skeletonCooldown[],bool skeletonOnGround[], float skeletonVelY[],int skeleton_count, int cell_size, char skeletonFace[],const float jumpStrength)
{
    for (int s = 0; s < skeleton_count; s++)
    {

    if(skeleton_x[s] == 0 && skeleton_y[s] == 0)  // Skiping if captured
        continue; 
        // cooldown before next random decision
        skeletonCooldown[s]--;

        if (skeletonCooldown[s] <= 0)
        {
            int roll = rand() % 100; // 0–99

            if (roll < 50)
                skeletonState[s] = 0;            // 50% walking possibility
            else if (roll < 70)
                skeletonState[s] = 1;            // 20% stopping
            else
                skeletonState[s] = 2;            // 30%  jumping

            skeletonCooldown[s] = rand() % 60 + 40;
        }

        int tileX = (int)(skeleton_x[s] / cell_size);
        int tileY = (int)((skeleton_y[s] + 32) / cell_size); // mid-feet row
        int belowY = tileY + 1;

        // State 0 means WALKING state
        if (skeletonState[s] == 0)
        {
            float nextX = skeleton_x[s] + skeletonDir[s] * skeletonSpeed[s];
            int nextTileX = (int)(nextX / cell_size);

            // Checking for wall hitting only
            if (skeletonDir[s] == 1) // moving right
            {
                if (nextTileX + 1 >= width || lvl[tileY][nextTileX + 1] == '#')
                {
                    skeletonDir[s] *= -1;
                }
                else
                {
                    skeleton_x[s] = nextX;
                }
            }
            else // moving left
            {
                if (nextTileX < 0 || lvl[tileY][nextTileX] == '#')
                {
                    skeletonDir[s] *= -1;
                }
                else				
                {
                    skeleton_x[s] = nextX;
                }
            }
        }

        // State 1 means STOPING
        else if (skeletonState[s] == 1)
        {
            // stand still
        }

        // State 2 means JUMPING (similar to player)
        else if (skeletonState[s] == 2)
        {
            // only jump if standing on something and not on top row
            if (skeletonOnGround[s] && skeleton_y[s]!=12)
            {
                // reuse the jump() function
                jump(skeletonOnGround[s], skeletonVelY[s], jumpStrength*1.2);
            }

            // after one jump attempt, go back to walking
            skeletonState[s] = 0;
        }

        // facing direction for sprite flipping
        if (skeletonDir[s] == 1)
            skeletonFace[s] = 'R';
        else
            skeletonFace[s] = 'L';
    }
}

void spawningOrderLevel2(int enemyType[], int enemySpawnRowNum[], int enemySpawnColNum[], int& totalEnemies)
{
    int index = 0;
    
    // Platform row numbers where enemies can spawn
    int platforms[4] = {2, 5, 9, 12};
    
    // Columns where there are blocks i.e platform 
    int Cols[6] = {1,2,3,4,15,16};  // Columns with platforms
    int colCount = 6;
    
    // Adding ghosts
    for (int i = 0; i < 4; i++)
    {
        enemyType[index] = 0;
        enemySpawnRowNum[index] = platforms[rand() % 4];
        enemySpawnColNum[index] = Cols[rand() % colCount];
        index++;
    }
    
    // Adding skeletons
    for (int i = 0; i < 9; i++)
    {
        enemyType[index] = 1;
        enemySpawnRowNum[index] = platforms[rand() % 4];
        enemySpawnColNum[index] = Cols[rand() % colCount];
        index++;
    }
    
    // Adding invisible man
    for (int i = 0; i < 3; i++)
    {
        enemyType[index] = 2;
        enemySpawnRowNum[index] = platforms[rand() % 4];
        enemySpawnColNum[index] = Cols[rand() % colCount];
        index++;
    }
    
    // Adding chelnov
    for (int i = 0; i < 4; i++)
    {
        enemyType[index] = 3;
        enemySpawnRowNum[index] = platforms[rand() % 4];
        enemySpawnColNum[index] = Cols[rand() % colCount];
        index++;
    }

	/* ENEMY TYPE == 0 means ghosts
				  == 1 means skeletons
				  == 2 means chelnov
				  == 3 means invisible man*/
    
    totalEnemies = index; //20
    
    // Shuffling arrays for random spawning of enemies i.e, any enemy can be spawened, for example after chelnov skeleton can spawn, not necessarily another chelnov
    for (int i = 0; i < totalEnemies; i++)
    {
        int r = rand() % totalEnemies;
        
        // Swap types
        int tempType = enemyType[i];
        enemyType[i] = enemyType[r];
        enemyType[r] = tempType;
        
        // Swap rows
        int tempRow = enemySpawnRowNum[i];
        enemySpawnRowNum[i] = enemySpawnRowNum[r];
        enemySpawnRowNum[r] = tempRow;
        
        // Swap cols
        int tempCol = enemySpawnColNum[i];
        enemySpawnColNum[i] = enemySpawnColNum[r];
        enemySpawnColNum[r] = tempCol;
    }
}

// Spawn next enemy from queue
void spawnlevel2enemies(int spawnTypes[],int spawnRows[],int spawnCols[], int index,float ghostX[], float ghostY[], int ghostDir[], float ghostSpeed[],char ghostFace[], int ghost_count,float skeleton_x[], float skeleton_y[],float skeletonDir[], float skeletonSpeed[], int skeletonState[], int skeletonCooldown[], bool skeletonOnGround[], float skeleton_velocityY[], char skeletonFace[],int skeleton_count,
						float invisible_x[],float invisible_y[], float invisibleDir[], float invisibleSpeed[],char invisibleFace[], int invisible_count,int invisibleCooldown[],float chelnov_x[],float chelnov_y[], float chelnovDir[], float chelnovSpeed[], char chelnovFace[],int chelnov_count,const int cell_size){
    
	int enemyType = spawnTypes[index];
    float spawnXpoint = spawnCols[index] * cell_size;
    float spawnYpoint = spawnRows[index] * cell_size - 32;
    
    // Type 0 = Ghost
    if (enemyType == 0)
    {
        for (int g = 0; g < ghost_count; g++)
        {
            if (ghostX[g] == 0 && ghostY[g] == 0)
            {
                ghostX[g] = spawnXpoint;
                ghostY[g] = spawnYpoint;
                ghostDir[g] = 1;
                ghostSpeed[g] = 1.2f;
                ghostFace[g] = 'R';
                break;
            }
        }
    }
    // Type 1 = Skeleton
    else if (enemyType == 1)
    {
        for (int s = 0; s < skeleton_count; s++)
        {
            if (skeleton_x[s] == 0 && skeleton_y[s] == 0)
            {
                skeleton_x[s] = spawnXpoint;
                skeleton_y[s] = spawnYpoint;
                skeletonDir[s] = 1;
                skeletonSpeed[s] = 1.0f;
                skeletonState[s] = 0;
                skeletonCooldown[s] = rand() % 60 + 30;
                skeletonOnGround[s] = true;
                skeleton_velocityY[s] = 0.0f;
                skeletonFace[s] = 'R';
                break;
            }
        }
    }
    // Type 2 = Invisible man
    else if (enemyType == 2)
    {
        for (int i = 0; i < invisible_count; i++)
        {
            if (invisible_x[i] == 0 && invisible_y[i] == 0)
            {
                invisible_x[i] = spawnXpoint;
                invisible_y[i] = spawnYpoint;
                invisibleDir[i] = 1;
                invisibleSpeed[i] = 1.5f;
                invisibleFace[i] = 'R';
                invisibleCooldown[i] = 60;  // ← ADD THIS LINE
                break;
            }
        }
    }

    // Type 3 = Chelnov
    else if (enemyType == 3)
    {
        for (int c = 0; c < chelnov_count; c++)
        {
            if (chelnov_x[c] == 0 && chelnov_y[c] == 0)
            {
                chelnov_x[c] = spawnXpoint;
                chelnov_y[c] = spawnYpoint;
                chelnovDir[c] = 1;
                chelnovSpeed[c] = 1.3f;
                chelnovFace[c] = 'R';
                break;
            }
        }
    }
}


void moveInvisibleMen(char** lvl,int height,int width,float invisible_x[], float invisible_y[], float invisibleDir[], float invisibleSpeed[], int invisible_count,const int cell_size, char invisibleFace[],int invisibleTeleportCooldown[])
{
    for(int i = 0; i < invisible_count; i++)
    {
        if(invisible_x[i] == 0 && invisible_y[i] == 0)
            continue;
        
        // Calculate current position
        int invBlockX = (int)(invisible_x[i] / cell_size);
        int invBlockY = (int)(invisible_y[i] / cell_size);
        
        float predictedX = invisible_x[i] + (invisibleSpeed[i] * invisibleDir[i]);
        int nextTileX = (int)(predictedX / cell_size) + 32;
        
        // Wall check
        if (invisibleDir[i] == -1)
        {
            invisibleFace[i] = 'L';
            if(lvl[invBlockY][nextTileX] == '#')
            {
                invisibleDir[i] *= -1;
                invisibleFace[i] = 'R';
                continue;
            }
        }
        else
        {
            invisibleFace[i] = 'R';
            if(lvl[invBlockY][nextTileX + 1] == '#')
            {
                invisibleDir[i] *= -1;
                invisibleFace[i] = 'L';
                continue;
            }
        }
        
        // Floor check
        if(invisibleDir[i] == 1)
        {
            if(lvl[invBlockY + 1][nextTileX + 1] != '#')
            {
                invisibleDir[i] *= -1;
                continue;
            }
        }
        else
        {
            if(lvl[invBlockY + 1][nextTileX] != '#')
            {
                invisibleDir[i] *= -1;
                continue;
            }
        }
        
        // Move horizontally
        invisible_x[i] += invisibleSpeed[i] * invisibleDir[i];
    }
}

void moveChelnovs(char** lvl,int height,int width, float chelnov_x[],float chelnov_y[], float chelnovDir[],float chelnovSpeed[],int chelnov_count,const int cell_size, char chelnovFace[])
{
    for(int i = 0; i < chelnov_count; i++)
    {
        if(chelnov_x[i] == 0 && chelnov_y[i] == 0)
            continue;
        
        int invBlockX = (int)(chelnov_x[i] / cell_size);
        int invBlockY = (int)(chelnov_y[i] / cell_size);
        
        float predictedX = chelnov_x[i] + (chelnovSpeed[i] * chelnovDir[i]);
        int nextTileX = (int)(predictedX / cell_size) + 32;
        
        // Wall check
        if (chelnovDir[i] == -1)
        {
            chelnovFace[i] = 'L';
            if(lvl[invBlockY][nextTileX] == '#')
            {
                chelnovDir[i] *= -1;
                chelnovFace[i] = 'R';
                continue;
            }
        }
        else
        {
            chelnovFace[i] = 'R';
            if(lvl[invBlockY][nextTileX + 1] == '#')
            {
                chelnovDir[i] *= -1;
                chelnovFace[i] = 'L';
                continue;
            }
        }
        
        // Floor check
        if(chelnovDir[i] == 1)
        {
            if(lvl[invBlockY + 1][nextTileX + 1] != '#')
            {
                chelnovDir[i] *= -1;
                continue;
            }
        }
        else
        {
            if(lvl[invBlockY + 1][nextTileX] != '#')
            {
                chelnovDir[i] *= -1;
                continue;
            }
        }
        
        chelnov_x[i] += chelnovSpeed[i] * chelnovDir[i];
    }
}

void initLevel1enemies(float ghostX[], float ghostY[], int ghostDir[], float ghostSpeed[], int ghost_count, int ghostTimer[], float skeleton_x[], float skeleton_y[],float skeletonDir[], float skeletonSpeed[],int skeleton_count, const int cell_size,int skeletonState[],int skeletonCooldown[],bool skeletonOnGround[], float skeletonY_velocity[]){
	initSkeletons ( skeleton_x,  skeleton_y, skeletonDir, skeletonSpeed, skeleton_count, cell_size, skeletonState, skeletonCooldown, skeletonOnGround, skeletonY_velocity);
	initGhosts (ghostX, ghostY, ghostDir, ghostSpeed, ghost_count, ghostTimer);
}

void slide(char** lvl, float& player_x, int PlayerHeight, int PlayerWidth, const int cell_size, float slideSpeed, bool onGround, float& player_y, bool& onSlope)
{
    // Only slide if on ground
    if (!onGround)
    {
        onSlope = false;
        return;
    }
    
    // Check if standing on slanted platform
    char bottom_left = lvl[(int)(player_y + PlayerHeight) / cell_size][(int)(player_x) / cell_size];
    char bottom_mid = lvl[(int)(player_y + PlayerHeight) / cell_size][(int)(player_x + PlayerWidth/2) / cell_size];
    char bottom_right = lvl[(int)(player_y + PlayerHeight) / cell_size][(int)(player_x + PlayerWidth) / cell_size + 1];
    
    if (bottom_right == '!' )
    {
        onSlope = true;
        player_x --;
		player_y ++;
    }
    else
    {
        onSlope = false;
    }
}

void platformClear(char** lvl, const int height, const int width){
	for (int i = 0; i < height; i++)
	{
		for (int j=0;  j< width; j++)
		{
			if (i == 0 || j == 0 || j == width - 1 || i == height - 1);
			else 
				lvl[i][j] = ' ';
		}	
	}
}

				//PLATFORM FORMATION
void platform(char**lvl, const int height, const int width){
	for (int i = 0; i < height; i++)
	{
		for (int j=0;  j< width; j++)
		{
			if (( i==3 ) && ( j>2 && j<width-3 ) )
				lvl[i][j] = '#';

			if ( (i==6 || i==10) && (j<=5 || j>=width-6) )
				lvl[i][j] = '#';

			lvl[8][8] = '#';
			lvl[8][9] = '#';
			
		}	
	}
}

void jump(bool& onGround, float& velocityY, const float jumpStrength){
	// It initializes Jump i.e, gives Initial velocity for jump. Rest of motion handling is done in gravity function.
	if(onGround){
		velocityY = jumpStrength;
		onGround = false;
	}
}

void fall(bool& onGround, float& player_y, const float jumpStrength, int PlayerHeight, const int cell_size){
	if(onGround){
		{
			if (player_y + PlayerHeight < screen_y - (cell_size * 2)) // Clamping down movement, cannot move down if standing on last bottom boundary
			player_y -= jumpStrength;
		}
	}
}

void player_gravity(char** lvl, float& offset_y, float& velocityY, bool& onGround, const float& gravity, float& terminal_Velocity, float& player_x, float& player_y, const int cell_size, int& Pheight, int& Pwidth, bool& onSlope)
{
    // Store the initial y position of player
    float original_y = player_y;

    offset_y = player_y;
    offset_y += velocityY; 

	bool isJumping = true; // Gravity only acts when player is not on platform i.e, it is in jump.

    char bottom_left_down = lvl[(int)(offset_y + Pheight) / cell_size][(int)(player_x ) / cell_size];
    char bottom_right_down = lvl[(int)(offset_y  + Pheight) / cell_size][(int)(player_x + Pwidth) / cell_size];
    char bottom_mid_down = lvl[(int)(offset_y + Pheight) / cell_size][(int)(player_x + Pwidth / 2) / cell_size];

    // Block below player must be # and the velocity Y of player should be +ve to land on block and stop motion. 
	// If velocity Y is -ve it means player is moving upwards and platforms check are not required and onGround bool should remain false so that the player does'nt get stuck in the platform.
    if ( (bottom_left_down == '#' || bottom_mid_down == '#' || bottom_right_down == '#' || bottom_left_down == '!' || bottom_mid_down == '!' || bottom_right_down == '!') && velocityY > 0)
    {
        // Calculating the top edge of block where player is standing after moving vertically.
        float block_top_y = ((int)(offset_y + Pheight) / cell_size) * cell_size;
        
        // Checking if the player's bottom edge at the original position was above the block's top edge where player has moved.
        // It verifies that if player is landing on block and not hitting it from any side. Without this check player can get stuck inside the block if platform is above 3 blocks.
        if (original_y + Pheight <= block_top_y)
        {
            isJumping = false;
        }
    }
																		// BOUNDARY CHECK CLAMP
																	if (velocityY < 0 && player_y < 64)
																		velocityY = 0;	

    if (!isJumping)
    {
        onGround = true;
    }
    else
    {
        player_y = offset_y;
        onGround = false;
    }

    if (!onGround)
    {
        velocityY += gravity;
        if (velocityY >= terminal_Velocity) velocityY = terminal_Velocity;
    }

    else
    {
        velocityY = 0;
    }
}

void applySkeletonGravity(char** lvl,float skeleton_x[], float skeleton_y[],float skeletonVelY[], bool skeletonOnGround[],int skeleton_count,const float gravity, float terminal_velocity,const int cell_size){
    // Approximate skeleton size in pixels for collision
    int SkelHeight = 96; 
    int SkelWidth  = 48;
	bool temp = false;

    for (int s = 0; s < skeleton_count; s++)
    {
		if(skeleton_x[s] == 0 && skeleton_y[s] == 0)
        continue;
        float offset_y_dummy = 0.0f;
        player_gravity(lvl,offset_y_dummy,skeletonVelY[s],skeletonOnGround[s],gravity,terminal_velocity, skeleton_x[s], skeleton_y[s], cell_size, SkelHeight, SkelWidth,temp );
    }
}

void vacuum(char** lvl, float skeleton_x[], float skeleton_y[], float ghostX[], float ghostY[], float vaccumForce, int vaccumPower, float vaccumRange, char playerDirection, float player_x, float player_y, int PlayerWidth, int PlayerHeight, const int cell_size, int ghost_count, int skeleton_count, int& bag,int &score, int &comboCount, bool &comboActive, float &comboTimer)
{
    int playerBlockX = (int)(player_x / cell_size);
    int playerBlockY = (int)(player_y / cell_size);
    
    // Assigning vacuum range based on player's face direction
    int vaccumStarting, vaccumEnding;
    
    if (playerDirection == 'R')
    {
        vaccumStarting = playerBlockX;
        vaccumEnding = playerBlockX + vaccumPower;
    }
    else if (playerDirection == 'L')
    {
        vaccumStarting = playerBlockX - vaccumPower;
        vaccumEnding = playerBlockX;
    }
    else if (playerDirection == 'U')
    {
        vaccumStarting = playerBlockY - vaccumPower;
        vaccumEnding = playerBlockY;
    }
    else if (playerDirection == 'D')
    {
        vaccumStarting = playerBlockY;
        vaccumEnding = playerBlockY + vaccumPower;
    }
    else
    {
        return;
    }
    
    // GHOSTS SUCTION
    for (int g = 0; g < ghost_count; g++)
    {
        if (ghostX[g] == 0 && ghostY[g] == 0)
            continue;
        
        // Calculating the block number of ghost
        int ghostblockX = (int)(ghostX[g] / cell_size);
        int ghostblockY = (int)(ghostY[g] / cell_size);
        
        bool inRadius = false;
        bool inlength = false;
        
        // Check based on direction
        if (playerDirection == 'R')
		{
			// Check if ghost is within vaccumRange vertically
			if (( player_y - ghostY[g] <= vaccumRange && player_y - ghostY[g] >= 0) ||(ghostY[g] - player_y <= vaccumRange && ghostY[g] - player_y >= 0))
				inRadius = true;
			// Check in enemy is withing horizontal length
			if (ghostblockX >= vaccumStarting && ghostblockX <= vaccumEnding && ghostX[g] > player_x)
				inlength = true;
		}
        else if (playerDirection == 'L')
		{
			if (( player_y - ghostY[g] <= vaccumRange && player_y - ghostY[g] >= 0) ||(ghostY[g] - player_y <= vaccumRange && ghostY[g] - player_y >= 0))
				inRadius = true;
			
			if (ghostblockX >= vaccumStarting && ghostblockX <= vaccumEnding && ghostX[g] < player_x)
				inlength = true;
		}
        else if (playerDirection == 'U')
		{
			if ((player_x - ghostX[g] <= vaccumRange && player_x - ghostX[g] >= 0) || (ghostX[g] - player_x <= vaccumRange && ghostX[g] - player_x >= 0))
				inRadius = true;
			
			if (ghostblockY >= vaccumStarting && ghostblockY <= vaccumEnding && ghostY[g] < player_y)
				inlength = true;
		}
        else if (playerDirection == 'D')
		{
			if ((player_x-ghostX[g] <= vaccumRange && player_x-ghostX[g] >= 0) || (ghostX[g]-player_x <= vaccumRange && ghostX[g]-player_x >= 0))
				inRadius = true;
			
			if (ghostblockY >= vaccumStarting && ghostblockY <= vaccumEnding && ghostY[g] > player_y)
				inlength = true;
		}
        
        // Only apply vacuum if ghost is both in range AND in power
        if (inRadius && inlength)
        {
            // PULL GHOST TOWARDS THE PLAYER
            if (playerDirection == 'R')
            {
                ghostX[g] += vaccumForce;
            }
            else if (playerDirection == 'L')
            {
                ghostX[g] -= vaccumForce;
            }
            else if (playerDirection == 'U')
            {
                ghostY[g] -= vaccumForce;
            }
            else if (playerDirection == 'D')
            {
                ghostY[g] += vaccumForce;
            }
            
            // Check if ghost has reached the player to capture it
            if (playerDirection == 'R')
            {
                if (ghostX[g] <= player_x + PlayerWidth)
                {
                    ghostX[g] = 0;
                    ghostY[g] = 0;
					bag++;
					// SCORE: Capture Ghost = 50
					int points = 50;

					// Applying combo multiplier
					if (comboCount >= 3 && comboCount <= 4)
						points = (int)(points * 1.5f);
					else if (comboCount >= 5)
						points = (int)(points * 2.0f);

					score += points;

					// Updating combo state
					comboCount++;
					comboActive = true;
					comboTimer = 0.0f;
                }
            }
            else if (playerDirection == 'L')
            {
                if (ghostX[g] >= player_x)
                {
                    ghostX[g] = 0;
                    ghostY[g] = 0;
					bag++;
					// SCORE: Capture Ghost = 50
					int points = 50;

					if (comboCount >= 3 && comboCount <= 4)
						points = (int)(points * 1.5f);
					else if (comboCount >= 5)
						points = (int)(points * 2.0f);

					score += points;

					comboCount++;
					comboActive = true;
					comboTimer = 0.0f;
                }
            }
            else if (playerDirection == 'U')
            {
                if (ghostY[g] >= player_y - 10)
                {
                    ghostX[g] = 0;
                    ghostY[g] = 0;
					bag++;
					// SCORE: Capture Ghost = 50
					int points = 50;

					if (comboCount >= 3 && comboCount <= 4)
						points = (int)(points * 1.5f);
					else if (comboCount >= 5)
						points = (int)(points * 2.0f);

					score += points;

					comboCount++;
					comboActive = true;
					comboTimer = 0.0f;
                }
            }
            else if (playerDirection == 'D')
            {
                if (ghostY[g] <= player_y + PlayerHeight)
                {
                    ghostX[g] = 0;
                    ghostY[g] = 0;
					bag++;

					int points = 50;

					if (comboCount >= 3 && comboCount <= 4)
						points = (int)(points * 1.5f);
					else if (comboCount >= 5)
						points = (int)(points * 2.0f);

					score += points;

					comboCount++;
					comboActive = true;
					comboTimer = 0.0f;
                }
            }
        }
    }
    
    // SKELETONS SUCTION
    for (int s = 0; s < skeleton_count; s++)
    {
        // Skip the iteration if skeleton is already captured
        if (skeleton_x[s] == 0 && skeleton_y[s] == 0)
            continue;
        
        int skeletonblockX = (int)(skeleton_x[s] / cell_size);
        int skeletonblockY = (int)(skeleton_y[s] / cell_size);
        
        bool inRadius = false;
        bool inlength = false;
        
        if (playerDirection == 'R')
        {
            if ((player_y-skeleton_y[s] <= vaccumRange && player_y - skeleton_y[s] >= 0) ||(skeleton_y[s] -player_y <= vaccumRange && skeleton_y[s] - player_y >= 0))
                inRadius = true;
            
            if (skeletonblockX >= vaccumStarting && skeletonblockX <= vaccumEnding && skeleton_x[s] > player_x)
                inlength = true;
        }
        else if (playerDirection == 'L')
        {
            if ((player_y-skeleton_y[s] <= vaccumRange && player_y - skeleton_y[s] >= 0) ||(skeleton_y[s] -player_y <= vaccumRange && skeleton_y[s] - player_y >= 0))
                inRadius = true;
            
            if (skeletonblockX >= vaccumStarting && skeletonblockX <= vaccumEnding && skeleton_x[s] < player_x)
                inlength = true;
        }
        else if (playerDirection == 'U')
        {
            if ((player_x - skeleton_x[s] <= vaccumRange && player_x - skeleton_x[s] >= 0) || (skeleton_x[s] - player_x <= vaccumRange && skeleton_x[s] - player_x >= 0))
                inRadius = true;
            
            if (skeletonblockY >= vaccumStarting && skeletonblockY <= vaccumEnding && skeleton_y[s] < player_y)
                inlength = true;
        }
        else if (playerDirection == 'D')
        {
            if ((player_x - skeleton_x[s] <= vaccumRange && player_x - skeleton_x[s] >= 0) || (skeleton_x[s] - player_x <= vaccumRange && skeleton_x[s] - player_x >= 0))
                inRadius = true;
            
            if (skeletonblockY >= vaccumStarting && skeletonblockY <= vaccumEnding && skeleton_y[s] > player_y)
                inlength = true;
        }
        
        if (inRadius && inlength)
        {
            // PULL SKELETON TOWARDS THE PLAYER
            if (playerDirection == 'R')
            {
                skeleton_x[s] += vaccumForce;
            }
            else if (playerDirection == 'L')
            {
                skeleton_x[s] -= vaccumForce;
            }
            else if (playerDirection == 'U')
            {
                skeleton_y[s] -= vaccumForce;
            }
            else if (playerDirection == 'D')
            {
                skeleton_y[s] += vaccumForce*1.8;
            }
            
            // Check if skeleton has reached the player to capture it
            if (playerDirection == 'R')
            {
                if (skeleton_x[s] <= player_x + PlayerWidth)
                {
                    skeleton_x[s] = 0;
                    skeleton_y[s] = 0;
					bag++;
					int points = 75; // skeleton capture

					if (comboCount >= 3 && comboCount <= 4)
						points = (int)(points * 1.5f);
					else if (comboCount >= 5)
						points = (int)(points * 2.0f);

					score += points;

					comboCount++;
					comboActive = true;
					comboTimer = 0.0f;
                }
            }
            else if (playerDirection == 'L')
            {
                if (skeleton_x[s] >= player_x)
                {
                    skeleton_x[s] = 0;
                    skeleton_y[s] = 0;
					bag++;
					int points = 75; // skeleton capture

					if (comboCount >= 3 && comboCount <= 4)
						points = (int)(points * 1.5f);
					else if (comboCount >= 5)
						points = (int)(points * 2.0f);

					score += points;

					comboCount++;
					comboActive = true;
					comboTimer = 0.0f;
                }
            }
            else if (playerDirection == 'U')
            {
                if (skeleton_y[s] >= player_y - 10)
                {
                    skeleton_x[s] = 0;
                    skeleton_y[s] = 0;
					bag++;
					int points = 75; // skeleton capture

					if (comboCount >= 3 && comboCount <= 4)
						points = (int)(points * 1.5f);
					else if (comboCount >= 5)
						points = (int)(points * 2.0f);

					score += points;

					comboCount++;
					comboActive = true;
					comboTimer = 0.0f;
                }
            }
            else if (playerDirection == 'D')
            {
                if (skeleton_y[s] <= player_y + PlayerHeight)
                {
                    skeleton_x[s] = 0;
                    skeleton_y[s] = 0;
					bag++;
					int points = 75; // skeleton capture

					if (comboCount >= 3 && comboCount <= 4)
						points = (int)(points * 1.5f);
					else if (comboCount >= 5)
						points = (int)(points * 2.0f);

					score += points;

					comboCount++;
					comboActive = true;
					comboTimer = 0.0f;
                }
            }
        }
    }
}

void killPlayer(float &player_x, float &player_y,int &playerLives, bool &playerInvulnerable,Clock &invClock, float respawnX, float respawnY, int& bag,int& score)
{
    playerLives--;
	bag = 0;

    // Respawn player
    player_x = respawnX;
    player_y = respawnY;

    // Start invulnerability
    playerInvulnerable = true;
    invClock.restart();
	score-=50;
	if(playerLives==0)
	{
		score-=200;
	}
}

void platform2(char** lvl, const int height, const int width)
{
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if ((i == 3) && (j <= 4 || j > width - 5))
                lvl[i][j] = '#';
			if ((i==6 || i == 10) && (j<5 || j>width-5))
				lvl[i][j] = '#';
			
        }  
    }
    
    int startRow = 11;
    int startCol = 5;
    for (int i = 0; i < 9; i++)
    {
        lvl[startRow - i][startCol + i] = '!';
        lvl[startRow - i][startCol + 1 + i] = '!';
    }
}

bool levelCompletionCheck (float ghostX[], float ghostY[], float skeleton_x[], float skeleton_y[], 
                          float invisible_x[], float invisible_y[], float chelnov_x[], float chelnov_y[],
                          int ghost_count, int skeleton_count, int invisible_count, int chelnov_count, int levelNum)
{
    // Check ghosts
	if (levelNum == 1)
	{
		for (int i = 0; i < ghost_count; i++)
		{
			if (ghostX[i] != 0 || ghostY[i] != 0)
				return false;
		}
		
		// Check skeletons
		for (int i = 0; i < skeleton_count; i++)
		{
			if (skeleton_x[i] != 0 || skeleton_y[i] != 0)
				return false;
		}
	}
    // For level 2, also check new enemies
    if (levelNum == 2)
    {
        for (int i = 0; i < invisible_count; i++)
        {
            if (invisible_x[i] != 0 || invisible_y[i] != 0)
                return false;
        }
        
        for (int i = 0; i < chelnov_count; i++)
        {
            if (chelnov_x[i] != 0 || chelnov_y[i] != 0)
                return false;
        }
		for (int i = 0; i < 4; i++)
		{
			if (ghostX[i] != 0 || ghostY[i] != 0)
				return false;
		}
		
		// Check skeletons
		for (int i = 0; i < 9; i++)
		{
			if (skeleton_x[i] != 0 || skeleton_y[i] != 0)
				return false;
		}
    }
    
    return true;
}

int main()
{
	int levelNum = 1;
	bool levelCompleted = false;

	srand(time(NULL));
	RenderWindow window(VideoMode(screen_x, screen_y), "Tumble-POP", Style::Resize);
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);

																	//level specifics
	const int cell_size = 64;
	const int height = 14;
	const int width = 18;
	char** lvl;

	char playerDirection = '\0';

														//level and background textures and sprites
	Texture bgTex;
	Sprite bgSprite;
	Texture blockTexture;
	Sprite blockSprite;

	bgTex.loadFromFile("Data/bg.jpeg");
	bgSprite.setTexture(bgTex);
	bgSprite.setPosition(0,0);

	blockTexture.loadFromFile("Data/block1.png");
	blockSprite.setTexture(blockTexture);

																//Music initialisation
	Music lvlMusic;

	lvlMusic.openFromFile("Data/mus.ogg");
	lvlMusic.setVolume(20);
	lvlMusic.play();
	lvlMusic.setLoop(true);

																	//player data
	float player_x = 64;
	float player_y = 64;
	int playerLives = 3;
	
	bool playerInvulnerable = false;
	float invTimer = 0.0f;

	float respawnX = 64;           
	float respawnY = 896-192;
	
	Clock invClock;


	float speed = 5;

	const float jumpStrength = -20; // Initial jump velocity
	const float gravity = 1;  // Gravity acceleration

	bool isJumping = false;  // Track if jumping
	bool isMoving = false; //Motion detection

	bool up_collide = false;
	bool left_collide = false;
	bool right_collide = false;

	Texture PlayerTexture;
	Sprite PlayerSprite;

	bool onGround = false;

	float offset_x = 0;
	float offset_y = 0;
	float velocityY = 0;

	float terminal_Velocity = 20;

	int PlayerHeight = 102;
	int PlayerWidth = 96;

	int horizontal_index = 0;

	bool up_button = false;

	char top_left = '\0';
	char top_right = '\0';
	char top_mid = '\0';

	char left_mid = '\0';
	char left_bottom = '\0';
	char left_top = '\0';

	char right_top = '\0';
	char right_mid = '\0';
	char right_bottom = '\0';

	char bottom_left = '\0';
	char bottom_right = '\0';
	char bottom_mid = '\0';

	char bottom_left_down = '\0';
	char bottom_right_down = '\0';
	char bottom_mid_down = '\0';

	char top_right_up = '\0';
	char top_mid_up = '\0';
	char top_left_up = '\0';
																		//SLANT VARIABLES
	bool onSlope = false;
	float slideSpeed = 3.0f;

																		//VACCUM VARIABLES
	float vaccumForce = -8.0f;
	float vaccumRange = 64.0f;
	float vaccumPower = 3.0f;
	int bag = 0;

	Texture ballTex;
	ballTex.loadFromFile("Data/ball.png");
	Sprite ballSprite;
	ballSprite.setTexture(ballTex);
	ballSprite.setScale(0.4f,0.4f);
	char shootDir = 'R';



																	//GHOST VARIABLES and SPRITE
	const int ghost_count = 8;

	float ghostX[ghost_count];
	float ghostY[ghost_count];
	int ghostDir[ghost_count];   // 1 = right, -1 = left
	float ghostSpeed[ghost_count];
	int ghostTimer[ghost_count];
	char ghostFace[ghost_count];

	Texture ghostTex;
	Sprite ghostSprite;

	ghostTex.loadFromFile("Data/ghost.png");
	ghostSprite.setTexture(ghostTex);
	ghostSprite.move(32,0);
	ghostSprite.setTextureRect(IntRect(0, 0, 64, 64));

																		//SKELETON VARIABLES
	const int skeleton_count = 4;

	// positions
	float skeleton_x[skeleton_count];
	float skeleton_y[skeleton_count];
	float skeletonSpeed[skeleton_count];
	float skeletonDir[skeleton_count];
	int skeletonState[skeleton_count];    // 0 = walk, 1 = stop, 2 = vertical move
	int skeletonCooldown[skeleton_count]; // frames until next decision
	char skeletonFace[skeleton_count];
	bool  skeletonOnGround[skeleton_count];
	float skeletonY_velocity[skeleton_count];

																	// INVISIBLE MAN VARIABLES

	Texture invManTex;
	Sprite invManSprite;

	const int invisible_count = 3;

    float invisible_x[invisible_count];
    float invisible_y[invisible_count];
    float invisibleDir[invisible_count];
    float invisibleSpeed[invisible_count];
    char invisibleFace[invisible_count];
	int invisibleTeleportCooldown[invisible_count];

	invManTex.loadFromFile("Data/IM.png");
	invManSprite.setTexture(invManTex);
	invManSprite.setScale(2,2);
	invManSprite.setTextureRect(IntRect(0, 0, 64, 64));
	invManSprite.setPosition(128, 256);

																	// CHELNOV VARIABLES
	const int chelnov_count = 4;

    float chelnov_x[chelnov_count];
    float chelnov_y[chelnov_count];
    float chelnovDir[chelnov_count];
    float chelnovSpeed[chelnov_count];
    char chelnovFace[chelnov_count];

	Texture chelnovTex;
	Sprite chelnovSprite;

	chelnovTex.loadFromFile("Data/chelnov.png");
	chelnovSprite.setTexture(chelnovTex);
	chelnovSprite.setScale(2,2);
	chelnovSprite.setTextureRect(IntRect(0, 0, 64, 64));
	chelnovSprite.setPosition(128, 256);

	//Initializing every array to 0
	for(int i = 0; i < ghost_count; i++) { ghostX[i] = 0; ghostY[i] = 0; }
    for(int i = 0; i < skeleton_count; i++) { skeleton_x[i] = 0; skeleton_y[i] = 0; }
    for(int i = 0; i < invisible_count; i++) { invisible_x[i] = 0; invisible_y[i] = 0; invisibleTeleportCooldown[i] = 60;}
    for(int i = 0; i < chelnov_count; i++) { chelnov_x[i] = 0; chelnov_y[i] = 0; }


																//LEVEL 2 ENEMIES DATA FOR FUNCTION
	const int max_spawn = 20;
    int spawnTypes[max_spawn];   // 0=ghost, 1=skeleton, 2=invisible, 3=chelnov
    int spawnRows[max_spawn];    // Which row to spawn at
    int spawnCols[max_spawn];    // Which column to spawn at
    int totalSpawns = 0;
    int currentSpawnIndex = 0;
    
    int spawnTimer = 0;
    int spawnInterval = 240;  // 240 frames means 4 seconds as 60 FPS

	if(levelNum == 1)
	initLevel1enemies(ghostX, ghostY, ghostDir, ghostSpeed, ghost_count,ghostTimer, skeleton_x,skeleton_y,skeletonDir,skeletonSpeed,skeleton_count,cell_size,skeletonState,skeletonCooldown,skeletonOnGround,skeletonY_velocity);
	
	
	// sprites + texture
	Texture skeletonTexture;
	Sprite skeletonSprite;

	skeletonTexture.loadFromFile("Data/Skelton.png");
	skeletonSprite.setTexture(skeletonTexture);
	skeletonSprite.setTextureRect(IntRect(0, 0, 48, 48));


																		//PLAYER SPRITE
	PlayerTexture.loadFromFile("Data/player.png");
	PlayerSprite.setTexture(PlayerTexture);
	PlayerSprite.setScale(3,3);
	PlayerSprite.setPosition(player_x, player_y);

	// BALL PROJECTILE VARIABLES
	bool ballActive = false;
	float ballX = 0, ballY = 0;
	float ballVelX = 0, ballVelY = 0;
	float ballSpeed = 10.0f;   // horizontal speed
	float ballGravity = 0.8f;  // downward curved movement
	float ballBounceFactor = -0.8f; // bounce off walls/roof

	// SCORE DISPLAY
	Font scoreFont;
	Text scoreText;
	scoreFont.loadFromFile("Data/ARIAL.TTF");
	scoreText.setFont(scoreFont);
	scoreText.setCharacterSize(32);          // Size of text
	scoreText.setFillColor(Color::White);    // Text color
	scoreText.setPosition(64, 90);       // Top-left corner
	scoreText.setString("Score: 0");

	Clock levelClock;
	float levelTime = 0.0f;

	//SCORE VARIABLES
	int score = 0;
	int comboCount = 0;
	float comboTimer = 0.0f;
	bool comboActive = false;
	int killedThisShot;

	bool showTitleScreen = true;   
	bool showCharacterSelect = false; 
	int chosenCharacter = -1;   // 0 = Yellow, 1 = Green


	Texture titleTex;
	Sprite titleSprite;

	titleTex.loadFromFile("Data/title.jpg");     // your title screen
	titleSprite.setTexture(titleTex);
	// SCALE TITLE SCREEN TO FILL THE WINDOW
	float scaleX = (float)screen_x / titleTex.getSize().x;
	float scaleY = (float)screen_y / titleTex.getSize().y;

	// Use the smaller scale to keep proportions clean
	float finalScale = (scaleX < scaleY) ? scaleX : scaleY;

	titleSprite.setScale(finalScale, finalScale);

	// Center it
	float posX = (screen_x - (titleTex.getSize().x * finalScale)) / 2;
	float posY = (screen_y - (titleTex.getSize().y * finalScale)) / 2;
	titleSprite.setPosition(posX, posY);

	// "Press Enter to Start" text
	Text startText;
	startText.setFont(scoreFont);
	startText.setCharacterSize(48);
	startText.setFillColor(Color::White);
	startText.setString("Press ENTER to Start");
	startText.setPosition(300, 700);

	Texture charSelectFullTex;
	Sprite charSelectSprite;

	charSelectFullTex.loadFromFile("Data/character.jpeg");
	charSelectSprite.setTexture(charSelectFullTex);

	//SCALE TO FIT SCREEN
	float charselectWidth = charSelectFullTex.getSize().x;
	float charselectHeight = charSelectFullTex.getSize().y;

	float charselectScaleX = (float)screen_x / charselectWidth;
	float charselectScaleY = (float)screen_y / charselectHeight;

	// keep aspect ratio clean
	float charselectFinalScale = (charselectScaleX < charselectScaleY) ? charselectScaleX : charselectScaleY;

	charSelectSprite.setScale(charselectFinalScale, charselectFinalScale);

	//CENTER ON SCREEN
	float charselectPosX = (screen_x - charselectWidth * charselectFinalScale) / 2;
	float charselectPosY = (screen_y - charselectHeight * charselectFinalScale) / 2;

	charSelectSprite.setPosition(charselectPosX, charselectPosY);

	//creating level array
	lvl = new char* [height];
	for (int i = 0; i < height; i += 1)
	{
		lvl[i] = new char[width];
	}

	boundaries(lvl, height, width);


	Event ev;
																		//MAIN LOOP
	while (window.isOpen())
	{
		// TITLE SCREEN 
		if (showTitleScreen)
		{
			while (window.pollEvent(ev))
			{
				if (ev.type == Event::Closed)
					window.close();

				if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Enter)
				{
					showTitleScreen = false;
					showCharacterSelect = true;
    				continue;
				}
			}

			window.clear();
			window.draw(titleSprite);
			window.draw(startText);
			window.display();
			continue;        
		}
		// CHARACTER SELECT SCREEN
		if (showCharacterSelect)
		{
			while (window.pollEvent(ev))
			{
				if (ev.type == Event::Closed)
					window.close();

				if (ev.type == Event::KeyPressed)
				{
					if (ev.key.code == Keyboard::Num0)
					{
						chosenCharacter = 0;    // Yellow
						showCharacterSelect = false;
					}
					if (ev.key.code == Keyboard::Num1)
					{
						chosenCharacter = 1;    // Green
						showCharacterSelect = false;
					}
				}
			}

			window.clear();
			window.draw(charSelectSprite);
			window.display();
			continue;

		}
		if (chosenCharacter == 0)
		{
			// Yellow character settings
			PlayerTexture.loadFromFile("Data/yellowsheet.png");
			PlayerSprite.setScale(3, 3);
			PlayerSprite.setTextureRect(IntRect(16,16,39,38));

			speed = 5;      //regular speed, more powerful vacuum
		}

		if (chosenCharacter == 1)
		{
			// Green character settings
			PlayerTexture.loadFromFile("Data/player.png");
			PlayerSprite.setScale(3, 3);

			speed = 7.5f;              // faster movement
		}
		while (window.pollEvent(ev))
		{
			if (ev.type == Event::Closed) 
			{
				window.close();
			}

			if (ev.type == Event::KeyPressed)
			{
			}

		}
		if (levelCompleted)
		{
			sleep(seconds(2));
			levelCompleted = false;
			playerLives = 3;
			levelNum += 1;
			skeleton_x[2] = 0;
			
				player_x = 64;
				player_y = 64;
				platformClear(lvl, height, width);
				platform2(lvl, height, width);

				if(levelNum == 2)
				{
					spawningOrderLevel2(spawnTypes, spawnRows, spawnCols, totalSpawns);
					currentSpawnIndex = 0;
					spawnTimer = 0;
				}
		}
		if (levelNum == 1)
			platform(lvl, height, width);

		if (levelNum == 2)
		{
			spawnTimer++;  // Increment every frame
			
			// Check if 4 seconds passed AND still have enemies to spawn
			if (spawnTimer >= spawnInterval && currentSpawnIndex < totalSpawns)
			{
				spawnlevel2enemies(spawnTypes,spawnRows,spawnCols,currentSpawnIndex,ghostX, ghostY, ghostDir, ghostSpeed, ghostFace, ghost_count,skeleton_x,skeleton_y,skeletonDir, skeletonSpeed,skeletonState,skeletonCooldown, skeletonOnGround,skeletonY_velocity, skeletonFace, skeleton_count,
							invisible_x,invisible_y, invisibleDir,invisibleSpeed,invisibleFace, invisible_count,invisibleTeleportCooldown,chelnov_x, chelnov_y, chelnovDir,chelnovSpeed,chelnovFace, chelnov_count,cell_size);
				
				currentSpawnIndex++;  // Move to next enemy
				spawnTimer = 0;        // Reset timer
			}
		}
		
		// UPDATE SHOOTING DIRECTION BASED ON WASD
		if (Keyboard::isKeyPressed(Keyboard::W)) shootDir = 'U';
		if (Keyboard::isKeyPressed(Keyboard::A)) shootDir = 'L';
		if (Keyboard::isKeyPressed(Keyboard::S)) shootDir = 'D';
		if (Keyboard::isKeyPressed(Keyboard::D)) shootDir = 'R';

		//presing escape to close
		if (Keyboard::isKeyPressed(Keyboard::Escape))
		{
			window.close();
		}

	if (!onSlope)
	{
		if (Keyboard::isKeyPressed(Keyboard::Left))
		{
			playerDirection = 'L';
													//COLLISION CHECK
			// Checking next bloxk if it is # or not to know whether to stop player or not.									
			char left_bottom = lvl[(int)(player_y + PlayerHeight) / cell_size][(int)(player_x - speed) / cell_size];
			char left_mid = lvl[(int)(player_y + PlayerHeight / 2) / cell_size][(int)(player_x - speed) / cell_size];
			char left_top = lvl[(int)(player_y) / cell_size][(int)(player_x - speed) / cell_size];

			// Check for movement. If next block is # then don't move the player
			if (onGround)
			{
				if (left_bottom != '#') {
					player_x -= speed;
				}

				// Now to exactly stop player at 13th block and another check if player is moving left while jumping to start projectile motion.
				else {
					if (!(Keyboard::isKeyPressed(Keyboard::Up)))
						player_x = ( ( (int)(player_x - speed) / cell_size) + 1) * cell_size;
					else 
					{
						if (player_x > cell_size)
							player_x -= speed;
					}
				}
			}
			else
			if (player_x > cell_size)
			player_x -= speed;
		}

		if (Keyboard::isKeyPressed(Keyboard::Right))
		{			
			playerDirection = 'R';
			right_bottom = lvl[(int)(player_y + PlayerHeight) / cell_size][(int)(player_x + PlayerWidth + speed) / cell_size];
			right_mid = lvl[(int)(player_y + PlayerHeight/2) / cell_size][(int)(player_x + PlayerWidth + speed) / cell_size];
			right_top = lvl[(int)(player_y) / cell_size][(int)(player_x + PlayerWidth + speed) / cell_size];

			if(right_bottom != '#')
				player_x += speed;
			else
			{
				if (!(Keyboard::isKeyPressed(Keyboard::Up)))
					player_x = (player_x / (int)cell_size)*cell_size;
				else
				{
					if (player_x + PlayerWidth < screen_x - cell_size - 10)
						player_x += speed;
				}
			}
		}
	
		if (Keyboard::isKeyPressed(Keyboard::Down))
		{
			fall(onGround, player_y, jumpStrength, PlayerHeight, cell_size);
		}
	}

		if (Keyboard::isKeyPressed(Keyboard::Up))
		{
			jump(onGround, velocityY, jumpStrength);
		}

		if (Keyboard::isKeyPressed(Keyboard::W))
		{
			playerDirection = 'U';
		}

		if (Keyboard::isKeyPressed(Keyboard::A))
		{
			playerDirection = 'L';
		}

		if (Keyboard::isKeyPressed(Keyboard::S))
		{
			playerDirection = 'D';
		}

		if (Keyboard::isKeyPressed(Keyboard::D))
		{
			playerDirection = 'R';
		}

		if (Keyboard::isKeyPressed(Keyboard::Space))
		{
			if (bag<3)
			vacuum(lvl, skeleton_x, skeleton_y, ghostX, ghostY, vaccumForce, (int)vaccumPower, vaccumRange, playerDirection, player_x, player_y, PlayerWidth, PlayerHeight, cell_size, ghost_count, skeleton_count, bag,score, comboCount, comboActive, comboTimer);
		}
		
																// LAUNCH ALL ENEMIES (E)
		if (Keyboard::isKeyPressed(Keyboard::E))
		{
			if (!ballActive && bag > 0)
			{
				ballActive = true;

				ballX = player_x;
				ballY = player_y;

					// BIG BALL: same direction as WASD aim, but stronger
				switch (shootDir)
				{
					case 'R':
						ballVelX = ballSpeed * 1.5f;
						ballVelY = -6;
						break;

					case 'L':
						ballVelX = -ballSpeed * 1.5f;
						ballVelY = -6;
						break;

					case 'U':
						ballVelX = 0;
						ballVelY = -14;
						break;

					case 'D':
						ballVelX = 0;
						ballVelY = 14;
						break;
				}
				bag = 0;// remove the captured enemies
    		}
		}

		// LAUNCH ONE ENEMY (LShift)
		if (Keyboard::isKeyPressed(Keyboard::LShift))
		{
			if (!ballActive && bag > 0)
			{
				ballActive = true;

				ballX = player_x;
				ballY = player_y;

				// LAUNCH USING WASD DIRECTION
				switch (shootDir)
				{
					case 'R':
						ballVelX = ballSpeed;
						ballVelY = -6;    // small arc upward
						break;

					case 'L':
						ballVelX = -ballSpeed;
						ballVelY = -6;
						break;

					case 'U':
						ballVelX = 0;
						ballVelY = -14;
						break;

					case 'D':
						ballVelX = 0;
						ballVelY = 14;
						break;
				}
				bag--;  // remove one enemy
			}
		}

		//calling function to move ghosts,skeletons

		moveGhosts(lvl, height, width, ghostX, ghostY, ghostDir, ghostSpeed, ghost_count, ghostFace);
		moveSkeletons(lvl, height, width,skeleton_x, skeleton_y, skeletonDir, skeletonSpeed, skeletonState, skeletonCooldown, skeletonOnGround, skeletonY_velocity, skeleton_count, cell_size, skeletonFace, jumpStrength);
		applySkeletonGravity(lvl, skeleton_x, skeleton_y, skeletonY_velocity, skeletonOnGround, skeleton_count, gravity, terminal_Velocity, cell_size);
		

		if (levelNum == 2)
		{
			moveInvisibleMen(lvl, height, width, invisible_x, invisible_y, 
                    invisibleDir, invisibleSpeed, invisible_count, 
                    cell_size, invisibleFace, invisibleTeleportCooldown);
			
			moveChelnovs(lvl, height, width, chelnov_x, chelnov_y, chelnovDir, chelnovSpeed, chelnov_count, cell_size, chelnovFace);
		}

		
											//BALL MOVEMENT
		if (ballActive)
		{
			// Move
			ballX += ballVelX;
			ballY += ballVelY;

			// Adding gravity ONLY for left/right shots so that projectile is made
			if (shootDir == 'L' || shootDir == 'R')
				ballVelY += ballGravity;

			//BOUNCE LOGIC
			
			// LEFT WALL
			if (ballX <= 0)
			{
				ballX = 0;
				ballVelX *= ballBounceFactor;
			}

			// RIGHT WALL
			if (ballX >= screen_x - 64)
			{
				ballX = screen_x - 64;
				ballVelX *= ballBounceFactor;
			}

			// CEILING
			if (ballY <= 0)
			{
				ballY = 0;
				ballVelY *= ballBounceFactor;
			}

			// FLOOR (destroy ball)
			if (ballY >= screen_y - 64)
			{
				ballActive = false;
			}
		}

										//  BALL COLLISION WITH ENEMIES 
		if (ballActive)
		{
			killedThisShot = 0;
			// Ghosts
			for (int g = 0; g < ghost_count; g++)
			{
				if (ghostX[g] == 0 && ghostY[g] == 0) continue;

				if (ballX + 32 > ghostX[g] &&
					ballX < ghostX[g] + 64 &&
					ballY + 32 > ghostY[g] &&
					ballY < ghostY[g] + 64)
				{
					ghostX[g] = 0;
					ghostY[g] = 0;
					score += 50 * 2;   // defeat by projectile = 2X points
					killedThisShot++;
					comboCount++;
					comboActive = true;
					comboTimer = 0.0f;
				}
			}

			// Skeletons
			for (int s = 0; s < skeleton_count; s++)
			{
				if (skeleton_x[s] == 0 && skeleton_y[s] == 0) continue;

				if (ballX + 32 > skeleton_x[s] &&
					ballX < skeleton_x[s] + 48 &&
					ballY + 32 > skeleton_y[s] &&
					ballY < skeleton_y[s] + 96)
				{
					skeleton_x[s] = 0;
					skeleton_y[s] = 0;
					score += 75 * 2;   // defeat by projectile = 2X points
					killedThisShot++;
					comboCount++;
					comboActive = true;
					comboTimer = 0.0f;
				}
			}
		}


										// Skip death check if invulnerable
		if (!playerInvulnerable)
		{
			// Check ghost collision
			for (int g = 0; g < ghost_count; g++)
			{
				if (ghostX[g] == 0 && ghostY[g] == 0) continue;

				if (player_x + PlayerWidth > ghostX[g] &&
					player_x < ghostX[g] + 64 &&
					player_y + PlayerHeight > ghostY[g] &&
					player_y < ghostY[g] + 64)
				{
					killPlayer(player_x, player_y, playerLives,
							playerInvulnerable, invClock,
							respawnX, respawnY, bag,score);
				}
			}

			// Check skeleton collision
			for (int s = 0; s < skeleton_count; s++)
			{
				if (skeleton_x[s] == 0 && skeleton_y[s] == 0) continue;

				if ( (player_x+PlayerWidth > skeleton_x[s] && player_x < skeleton_x[s]+48) && (player_y+ PlayerHeight > skeleton_y[s] &&player_y < skeleton_y[s] +96))
				{
					killPlayer(player_x, player_y, playerLives, playerInvulnerable, invClock, respawnX, respawnY, bag,score);
				}
			}
		}

		if (playerInvulnerable)
		{
			if (invClock.getElapsedTime().asSeconds() >= 3.0f)
				playerInvulnerable = false;
		}


		window.clear();

		display_level(window, lvl, bgTex, bgSprite, blockTexture, blockSprite, height, width, cell_size);
		slide (lvl, player_x, PlayerHeight, PlayerWidth, cell_size, slideSpeed, onGround, player_y, onSlope);

		if (!onSlope)
			player_gravity(lvl,offset_y,velocityY,onGround,gravity,terminal_Velocity, player_x, player_y, cell_size, PlayerHeight, PlayerWidth, onSlope);
		
		PlayerSprite.setPosition(player_x, player_y);
		if (playerDirection == 'R')
		{
			PlayerSprite.setScale(-3,3);
			PlayerSprite.move(PlayerWidth, 0);
		}
		else
			PlayerSprite.setScale(3,3);


		window.draw(PlayerSprite);

				//rendering all ghosts
		for(int g = 0; g < ghost_count; g++)
		{
			if (ghostX[g] == 0 && ghostY[g] == 0)
        	continue;

    		ghostSprite.setPosition(ghostX[g], ghostY[g]);
			if (ghostFace[g] == 'R')
			{
				ghostSprite.setScale(-2,2);
				ghostSprite.move(96,4);
			}
			else
			{
				ghostSprite.setScale(2,2);
				ghostSprite.move(-22,4);
			}
    		window.draw(ghostSprite);
		}
		for(int s = 0; s < skeleton_count; s++)
		{
			if (skeleton_x[s] == 0 && skeleton_y[s] == 0)
        	continue;
    		skeletonSprite.setPosition(skeleton_x[s], skeleton_y[s]);
			if (skeletonFace[s] == 'R')
			{
				skeletonSprite.setScale(-2,3);
				skeletonSprite.move(96,-48);
			}
			else
			{
				skeletonSprite.setScale(2,3);
				skeletonSprite.move(-22,-48);
			}
    		window.draw(skeletonSprite);
		}
													//RENDERING LEVEL 2 ENEMIES
		if (levelNum == 2)
		{
			for(int i = 0; i < invisible_count; i++)
			{
				if (invisible_x[i] != 0 || invisible_y[i] != 0)
				{
					invManSprite.setPosition(invisible_x[i], invisible_y[i]);
					if (invisibleFace[i] == 'R')
					{
						invManSprite.setScale(-2, 2);
						invManSprite.move(96, 4);
					}
					else
					{
						invManSprite.setScale(2, 2);
						invManSprite.move(-22, 4);
					}
					window.draw(invManSprite);
				}
			}
			
			// Render chelnovs
			for(int c = 0; c < chelnov_count; c++)
			{
				if (chelnov_x[c] != 0 || chelnov_y[c] != 0)
				{
					chelnovSprite.setPosition(chelnov_x[c], chelnov_y[c]);
					if (chelnovFace[c] == 'R')
					{
						chelnovSprite.setScale(-2, 2);
						chelnovSprite.move(96, 4);
					}
					else
					{
						chelnovSprite.setScale(2, 2);
						chelnovSprite.move(-22, 4);
					}
					window.draw(chelnovSprite);
				}
			}
		}

		if (ballActive)
		{
			ballSprite.setPosition(ballX, ballY);
			window.draw(ballSprite);
		}
		scoreText.setString("Score: " + to_string(score));

		window.draw(scoreText);


		levelCompleted = levelCompletionCheck(ghostX, ghostY, skeleton_x, skeleton_y, 
                                     invisible_x, invisible_y, chelnov_x, chelnov_y,
                                     ghost_count, skeleton_count, invisible_count, chelnov_count, levelNum);
		if (levelCompleted)
		{
			levelTime = levelClock.getElapsedTime().asSeconds();

			// LEVEL CLEAR BONUS
			if (levelNum == 1)
			{
				score += 1000;     // basic clear

				if (playerLives == 3)
					score += 1500; // no damage bonus

				if (levelTime < 60)      score += 500;
				else if (levelTime < 45) score += 1000;
				else if (levelTime < 30) score += 2000;
			}
			else if (levelNum == 2)
			{
				score += 2000;     // basic clear

				if (playerLives == 3)
					score += 2500; // no damage bonus

				if (levelTime < 120)     score += 750;
				else if (levelTime < 90) score += 1500;
				else if (levelTime < 60) score += 3000;
			}

			levelClock.restart();
		}
		window.display();
		if (playerLives <= 0)
		{
			window.close(); // or show game over screen later
		}
	}

	//stopping music and deleting level array
	lvlMusic.stop();
	for (int i = 0; i < height; i++)
	{
		delete[] lvl[i];
	}
	delete[] lvl;

	return 0;
	
}
