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

			if (lvl[i][j] == '#')
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
            ghostX[g] = (colStart + c * colStep)*64;   // spread horizontally
            ghostY[g] = (spawnRows[r]*64)-32;             // different rows
            ghostDir[g] = 1;                      // start moving right
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

		if(ghostX[g] == 0 && ghostY[g] == 0)  // Add this
        continue;

        int ghostPositionX = (int)(ghostX[g] / cell_size);
        int ghostPositionY = (int)(ghostY[g] / cell_size);

        float predictedX = ghostX[g] + (ghostMoveSpeed[g] * ghostDir[g]);
        int nextTileX = (int)(predictedX / cell_size)+32;

        // 1. Wall check
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

        // 2. Floor check
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

        // 3. Smooth pixel movement
        ghostX[g] += ghostMoveSpeed[g] * ghostDir[g];
    }
}

void initSkeletons(float skeleton_x[], float skeleton_y[],float skeletonDir[], float skeletonSpeed[],int skeleton_count, const int cell_size,int skeletonState[],int skeletonCooldown[],bool skeletonOnGround[], float skeletonY_velocity[])
{
    //one skeleton per row
    int spawnRows[4] = { 2, 5, 9, 12 }; 

    for (int i = 0; i < skeleton_count; i++)
    {
        skeleton_x[i] = 5 * cell_size;                // same column for all (column 5)
        skeleton_y[i] = (spawnRows[i] * cell_size) - 32;

        skeletonDir[i] = 1;          // facing right
        skeletonSpeed[i] = 1.0f;// future movement speed
		
		skeletonState[i] = 0;
		skeletonCooldown[i] = rand() % 60 + 30;   // 0.5–1.5 seconds before changing state

		skeletonOnGround[i] = true;
        skeletonY_velocity[i]     = 0.0f;
	}

}

void moveSkeletons(char** lvl, int height, int width,float skeleton_x[], float skeleton_y[],float skeletonDir[], float skeletonSpeed[],int skeletonState[], int skeletonCooldown[],bool skeletonOnGround[], float skeletonVelY[],int skeleton_count, int cell_size, char skeletonFace[],const float jumpStrength)
{
    for (int s = 0; s < skeleton_count; s++)
    {

    if(skeleton_x[s] == 0 && skeleton_y[s] == 0)  // Add this
        continue; 
        // cooldown before next random decision, added so skeletons dont go haywire and make 60 decisions every second
        skeletonCooldown[s]--;

        if (skeletonCooldown[s] <= 0)
        {
            int roll = rand() % 100; // 0–99

            if (roll < 50)
                skeletonState[s] = 0;            // 50% walk
            else if (roll < 70)
                skeletonState[s] = 1;            // 20% stop
            else
                skeletonState[s] = 2;            // 30% try jump

            skeletonCooldown[s] = rand() % 60 + 40;
        }

        int tileX = (int)(skeleton_x[s] / cell_size);
        int tileY = (int)((skeleton_y[s] + 32) / cell_size); // mid-feet row
        int belowY = tileY + 1;

        // State 0: WALK
        if (skeletonState[s] == 0)
        {
            float nextX = skeleton_x[s] + skeletonDir[s] * skeletonSpeed[s];
            int nextTileX = (int)(nextX / cell_size);

            // only wall check here if edge then they walk off and gravity handles drop
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

        // State 1: STOP
        else if (skeletonState[s] == 1)
        {
            // stand still
        }

        // State 2: JUMP (like player)
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
		if (player_y + PlayerHeight < screen_y - (cell_size * 2)) // Clamping down movement, cannot move down if standing on last bottom boundary
		player_y -= jumpStrength;
	}
}

void player_gravity(char** lvl, float& offset_y, float& velocityY, bool& onGround, const float& gravity, float& terminal_Velocity, float& player_x, float& player_y, const int cell_size, int& Pheight, int& Pwidth)
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
    if ( (bottom_left_down == '#' || bottom_mid_down == '#' || bottom_right_down == '#') && velocityY > 0)
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
    int SkelHeight = 96; // fits inside 64x64 tile
    int SkelWidth  = 48; // sprite width

    for (int s = 0; s < skeleton_count; s++)
    {
		if(skeleton_x[s] == 0 && skeleton_y[s] == 0)  // Add this
        continue;
        float offset_y_dummy = 0.0f;
        player_gravity(lvl,offset_y_dummy,skeletonVelY[s],skeletonOnGround[s],gravity,terminal_velocity, skeleton_x[s], skeleton_y[s], cell_size, SkelHeight, SkelWidth);
    }
}

void vacuum(char** lvl, float skeleton_x[], float skeleton_y[], float ghostX[], float ghostY[], 
            float vaccumForce, int vaccumPower, float vaccumRange, char playerDirection, float player_x, float player_y, 
            int PlayerWidth, int PlayerHeight, const int cell_size, int ghost_count, int skeleton_count, int& bag)
{
    // Calculate player's block position
    int playerBlockX = (int)(player_x / cell_size);
    int playerBlockY = (int)(player_y / cell_size);
    
    // Define vacuum range based on player direction
    int vaccumStartBlock, vaccumEndBlock;
    
    if (playerDirection == 'R')
    {
        vaccumStartBlock = playerBlockX;
        vaccumEndBlock = playerBlockX + vaccumPower;
    }
    else if (playerDirection == 'L')
    {
        vaccumStartBlock = playerBlockX - vaccumPower;
        vaccumEndBlock = playerBlockX;
    }
    else if (playerDirection == 'U')
    {
        vaccumStartBlock = playerBlockY - vaccumPower;
        vaccumEndBlock = playerBlockY;
    }
    else if (playerDirection == 'D')
    {
        vaccumStartBlock = playerBlockY;
        vaccumEndBlock = playerBlockY + vaccumPower;
    }
    else
    {
        return;
    }
    
    // GHOSTS SUCTION
    for (int g = 0; g < ghost_count; g++)
    {
        // Skip the iteration if ghost is already captured
        if (ghostX[g] == 0 && ghostY[g] == 0)
            continue;
        
        // Calculating the block number of ghost
        int ghostblockX = (int)(ghostX[g] / cell_size);
        int ghostblockY = (int)(ghostY[g] / cell_size);
        
        bool inRange = false;
        bool inPower = false;
        
        // Check based on direction
        if (playerDirection == 'R')
		{
			// Check if ghost is within vaccumRange vertically (without abs)
			if ((player_y - ghostY[g] <= vaccumRange && player_y - ghostY[g] >= 0) || 
				(ghostY[g] - player_y <= vaccumRange && ghostY[g] - player_y >= 0))
				inRange = true;
			
			if (ghostblockX >= vaccumStartBlock && ghostblockX <= vaccumEndBlock && ghostX[g] > player_x)
				inPower = true;
		}
        else if (playerDirection == 'L')
		{
			// Check if ghost is within vaccumRange vertically (without abs)
			if ((player_y - ghostY[g] <= vaccumRange && player_y - ghostY[g] >= 0) || 
				(ghostY[g] - player_y <= vaccumRange && ghostY[g] - player_y >= 0))
				inRange = true;
			
			if (ghostblockX >= vaccumStartBlock && ghostblockX <= vaccumEndBlock && ghostX[g] < player_x)
				inPower = true;
		}
        else if (playerDirection == 'U')
		{
			// Check if ghost is within vaccumRange horizontally (without abs)
			if ((player_x - ghostX[g] <= vaccumRange && player_x - ghostX[g] >= 0) || 
				(ghostX[g] - player_x <= vaccumRange && ghostX[g] - player_x >= 0))
				inRange = true;
			
			if (ghostblockY >= vaccumStartBlock && ghostblockY <= vaccumEndBlock && ghostY[g] < player_y)
				inPower = true;
		}
        else if (playerDirection == 'D')
		{
			// Check if ghost is within vaccumRange horizontally (without abs)
			if ((player_x - ghostX[g] <= vaccumRange && player_x - ghostX[g] >= 0) || 
				(ghostX[g] - player_x <= vaccumRange && ghostX[g] - player_x >= 0))
				inRange = true;
			
			if (ghostblockY >= vaccumStartBlock && ghostblockY <= vaccumEndBlock && ghostY[g] > player_y)
				inPower = true;
		}
        
        // Only apply vacuum if ghost is both in range AND in power
        if (inRange && inPower)
        {
            // PULL GHOST TOWARDS THE PLAYER
            if (playerDirection == 'R')
            {
                ghostX[g] += vaccumForce; // vaccumForce is negative, moves left
            }
            else if (playerDirection == 'L')
            {
                ghostX[g] -= vaccumForce; // subtract negative = moves right
            }
            else if (playerDirection == 'U')
            {
                ghostY[g] -= vaccumForce; // subtract negative = moves down
            }
            else if (playerDirection == 'D')
            {
                ghostY[g] += vaccumForce; // vaccumForce is negative, moves up
            }
            
            // Check if ghost has reached the player to capture it
            if (playerDirection == 'R')
            {
                if (ghostX[g] <= player_x + PlayerWidth)
                {
                    ghostX[g] = 0;
                    ghostY[g] = 0;
					bag++;
                }
            }
            else if (playerDirection == 'L')
            {
                if (ghostX[g] >= player_x)
                {
                    ghostX[g] = 0;
                    ghostY[g] = 0;
					bag++;
                }
            }
            else if (playerDirection == 'U')
            {
                if (ghostY[g] >= player_y - 10)
                {
                    ghostX[g] = 0;
                    ghostY[g] = 0;
					bag++;
                }
            }
            else if (playerDirection == 'D')
            {
                if (ghostY[g] <= player_y + PlayerHeight)
                {
                    ghostX[g] = 0;
                    ghostY[g] = 0;
					bag++;
                }
            }
        }
    }
    
    // SKELETONS SUCTION (same logic as ghosts)
    for (int s = 0; s < skeleton_count; s++)
    {
        // Skip the iteration if skeleton is already captured
        if (skeleton_x[s] == 0 && skeleton_y[s] == 0)
            continue;
        
        // Calculating the block number of skeleton
        int skeletonblockX = (int)(skeleton_x[s] / cell_size);
        int skeletonblockY = (int)(skeleton_y[s] / cell_size);
        
        bool inRange = false;
        bool inPower = false;
        
        // Check based on direction
        if (playerDirection == 'R')
        {
            if (abs(player_y - skeleton_y[s]) <= vaccumRange)
                inRange = true;
            
            if (skeletonblockX >= vaccumStartBlock && skeletonblockX <= vaccumEndBlock && skeleton_x[s] > player_x)
                inPower = true;
        }
        else if (playerDirection == 'L')
        {
            if (abs(player_y - skeleton_y[s]) <= vaccumRange)
                inRange = true;
            
            if (skeletonblockX >= vaccumStartBlock && skeletonblockX <= vaccumEndBlock && skeleton_x[s] < player_x)
                inPower = true;
        }
        else if (playerDirection == 'U')
        {
            if (abs(player_x - skeleton_x[s]) <= vaccumRange)
                inRange = true;
            
            if (skeletonblockY >= vaccumStartBlock && skeletonblockY <= vaccumEndBlock && skeleton_y[s] < player_y)
                inPower = true;
        }
        else if (playerDirection == 'D')
        {
            if (abs(player_x - skeleton_x[s]) <= vaccumRange)
                inRange = true;
            
            if (skeletonblockY >= vaccumStartBlock && skeletonblockY <= vaccumEndBlock && skeleton_y[s] > player_y)
                inPower = true;
        }
        
        // Only apply vacuum if skeleton is both in range AND in power
        if (inRange && inPower)
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
                }
            }
            else if (playerDirection == 'L')
            {
                if (skeleton_x[s] >= player_x)
                {
                    skeleton_x[s] = 0;
                    skeleton_y[s] = 0;
					bag++;
                }
            }
            else if (playerDirection == 'U')
            {
                if (skeleton_y[s] >= player_y - 10)
                {
                    skeleton_x[s] = 0;
                    skeleton_y[s] = 0;
					bag++;
                }
            }
            else if (playerDirection == 'D')
            {
                if (skeleton_y[s] <= player_y + PlayerHeight)
                {
                    skeleton_x[s] = 0;
                    skeleton_y[s] = 0;
					bag++;
                }
            }
        }
    }
}
void killPlayer(float &player_x, float &player_y,
                int &playerLives, bool &playerInvulnerable,
                Clock &invClock, float respawnX, float respawnY)
{
    playerLives--;

    // Respawn player
    player_x = respawnX;
    player_y = respawnY;

    // Start invulnerability
    playerInvulnerable = true;
    invClock.restart();
}



int main()
{
	int game = 0;

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

																		//VACCUM VARIABLES
	float vaccumForce = -8.0f;
	float vaccumRange = 64.0f;
	float vaccumPower = 3.0f;
	int bag = 0;

																	//GHOST VARIABLES and SPRITE
	const int ghost_count = 8;

	float ghostX[ghost_count];
	float ghostY[ghost_count];
	int ghostDir[ghost_count];   // 1 = right, -1 = left
	float ghostSpeed[ghost_count];
	int ghostTimer[ghost_count];
	char ghostFace[ghost_count];
	initGhosts(ghostX, ghostY, ghostDir, ghostSpeed, ghost_count,ghostTimer);


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


	initSkeletons(skeleton_x,skeleton_y,skeletonDir,skeletonSpeed,skeleton_count,cell_size,skeletonState,skeletonCooldown,skeletonOnGround,skeletonY_velocity);
	
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


	//creating level array
	lvl = new char* [height];
	for (int i = 0; i < height; i += 1)
	{
		lvl[i] = new char[width];
	}

	boundaries(lvl, height, width);
	platform(lvl, height, width);

	lvl[8][8] = '#';
	lvl[8][9] = '#';

	Event ev;
	//main loop
	while (window.isOpen())
	{

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

		//presing escape to close
		if (Keyboard::isKeyPressed(Keyboard::Escape))
		{
			window.close();
		}

		if ( (Keyboard::isKeyPressed(Keyboard::Left)) || (Keyboard::isKeyPressed(Keyboard::A)) )
		{
			playerDirection = 'L';
													//COLLISION CHECK
			// Checking next bloxk if it is # or not to know whether to stop player or not.									
			char left_bottom = lvl[(int)(player_y + PlayerHeight) / cell_size][(int)(player_x - speed) / cell_size];
			char left_mid = lvl[(int)(player_y + PlayerHeight / 2) / cell_size][(int)(player_x - speed) / cell_size];
			char left_top = lvl[(int)(player_y) / cell_size][(int)(player_x - speed) / cell_size];

			// Check for movement. If next block is # then don't move the player
			if (left_mid != '#' && left_bottom != '#' && left_top != '#') {
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

		if ( (Keyboard::isKeyPressed(Keyboard::Right)) || (Keyboard::isKeyPressed(Keyboard::D)) )
		{			
			playerDirection = 'R';
			right_bottom = lvl[(int)(player_y + PlayerHeight) / cell_size][(int)(player_x + PlayerWidth + speed) / cell_size];
			right_mid = lvl[(int)(player_y + PlayerHeight/2) / cell_size][(int)(player_x + PlayerWidth + speed) / cell_size];
			right_top = lvl[(int)(player_y) / cell_size][(int)(player_x + PlayerWidth + speed) / cell_size];

			if(right_mid != '#' && right_bottom != '#' && right_top != '#')
				player_x += speed;
			else
				if (!(Keyboard::isKeyPressed(Keyboard::Up)))
					player_x = (player_x / (int)cell_size)*cell_size;
				else
				{
					if (player_x + PlayerWidth < screen_x - cell_size - 10)
						player_x += speed;
				}
		}

		if (Keyboard::isKeyPressed(Keyboard::Up))
		{
			jump(onGround, velocityY, jumpStrength);
		}
		if (Keyboard::isKeyPressed(Keyboard::Down))
		{
			fall(onGround, player_y, jumpStrength, PlayerHeight, cell_size);
		}

		if (Keyboard::isKeyPressed(Keyboard::W))
		{
			playerDirection = 'U';
		}

		if (Keyboard::isKeyPressed(Keyboard::S))
		{
			playerDirection = 'D';
		}

		if (Keyboard::isKeyPressed(Keyboard::Space))
		{
			if (bag<3)
			vacuum(lvl, skeleton_x, skeleton_y, ghostX, ghostY, vaccumForce, (int)vaccumPower, vaccumRange, playerDirection, player_x, player_y, PlayerWidth, PlayerHeight, cell_size, ghost_count, skeleton_count, bag);
		}
		if (Keyboard::isKeyPressed(Keyboard::E))
		{
			bag = 0;
		}

		//calling function to move ghosts
		moveGhosts(lvl, height, width, ghostX, ghostY, ghostDir, ghostSpeed, ghost_count, ghostFace);
		moveSkeletons(lvl, height, width,skeleton_x, skeleton_y, skeletonDir, skeletonSpeed, skeletonState, skeletonCooldown, skeletonOnGround, skeletonY_velocity, skeleton_count, cell_size, skeletonFace, jumpStrength);
		applySkeletonGravity(lvl, skeleton_x, skeleton_y, skeletonY_velocity, skeletonOnGround, skeleton_count, gravity, terminal_Velocity, cell_size);
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
							respawnX, respawnY);
				}
			}

			// Check skeleton collision
			for (int s = 0; s < skeleton_count; s++)
			{
				if (skeleton_x[s] == 0 && skeleton_y[s] == 0) continue;

				if (player_x + PlayerWidth > skeleton_x[s] &&
					player_x < skeleton_x[s] + 48 &&
					player_y + PlayerHeight > skeleton_y[s] &&
					player_y < skeleton_y[s] + 96)
				{
					killPlayer(player_x, player_y, playerLives,
							playerInvulnerable, invClock,
							respawnX, respawnY);
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
		player_gravity(lvl,offset_y,velocityY,onGround,gravity,terminal_Velocity, player_x, player_y, cell_size, PlayerHeight, PlayerWidth);
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
		game = 0;
		for (int i=0; i<ghost_count; i++)
		{
			if(ghostX[i]==0 && ghostY[i]==0);
			else game++;
			if(i<skeleton_count)
			{
				if(skeleton_x[i]==0 && skeleton_y[i]==0);
				else game++;
			}
		}

		if (game);
		else
		 window.close();

		window.display();
		lvlMusic.stop();
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