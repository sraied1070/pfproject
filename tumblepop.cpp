#include <iostream>
#include <fstream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>

using namespace sf;
using namespace std;

int screen_x = 1152;
int screen_y = 896;

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

void initGhosts(int ghostX[], int ghostY[], int ghostDir[], int ghostSpeed[], int ghost_count,
				int ghostTimer[])
{
    // rows where ghosts can safely stand
    int spawnRows[4] = {2, 5, 8, 9};  // above platforms at 3, 6, 9, 10
    int rowCount = 4;

    int colStart = 3;     // starting column
    int colStep  = 3;     // spacing between ghosts

    int g = 0;

    // place two ghosts per row until we fill all 8
    for(int r = 0; r < rowCount && g < ghost_count; r++)
    {
        for(int c = 0; c < 2 && g < ghost_count; c++)
        {
            ghostX[g] = colStart + c * colStep;   // spread horizontally
            ghostY[g] = spawnRows[r];             // different rows
            ghostDir[g] = 1;                      // start moving right
            ghostSpeed[g] = 30;
			ghostTimer[g] = 0;
            g++;
        }
    }
}

void moveGhosts(char** lvl, int height, int width, 
                int ghostX[], int ghostY[], int ghostDir[], int ghostSpeed[], 
                int ghost_count, int ghostTimer[])
{
    for(int g = 0; g < ghost_count; g++)
    {
		ghostTimer[g]++;

        // move only every "ghostSpeed[g]" frames
        if(ghostTimer[g] < ghostSpeed[g])
            continue;

		ghostTimer[g]=0;

        int gx = ghostX[g];
        int gy = ghostY[g];
        int dir = ghostDir[g];

        int nextX = gx + dir;
        int nextY = gy;

        //turning around if hitting a wall
        if (lvl[nextY][nextX] == '#')
        {
            ghostDir[g] = -dir;
            continue;
        }

        //turn around if next tile below is not a block
        if (lvl[gy + 1][nextX] != '#')
        {
            ghostDir[g] = -dir;
            continue;
        }

        //move ghost
        ghostX[g] = nextX;
    }
}


				//PLATFORM FORMATION
void platform(char**lvl, const int height, const int width){
	for (int i = 0; i < height; i++)
	{
		for (int j=0;  j< width; j++)
		{
			if (( i==3 || i == 10) && ( j>2 && j<width-6 ) )
				lvl[i][j] = '#';

			if ( (i==6 || i==9) && (j<=3 || j>=width-4) )
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


int main()
{

	RenderWindow window(VideoMode(screen_x, screen_y), "Tumble-POP", Style::Resize);
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);

	//level specifics
	const int cell_size = 64;
	const int height = 14;
	const int width = 18;
	char** lvl;

	//level and background textures and sprites
	Texture bgTex;
	Sprite bgSprite;
	Texture blockTexture;
	Sprite blockSprite;

	bgTex.loadFromFile("Data/bg.png");
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

			//GHOST VARIABLES and SPRITE
	const int ghost_count = 8;

	int ghostX[ghost_count];
	int ghostY[ghost_count];
	int ghostDir[ghost_count];   // 1 = right, -1 = left
	int ghostSpeed[ghost_count];
	int ghostTimer[ghost_count];
	initGhosts(ghostX, ghostY, ghostDir, ghostSpeed, ghost_count,ghostTimer);


	Texture ghostTex;
	Sprite ghostSprite;

	ghostTex.loadFromFile("Data/ghost.png");
	ghostSprite.setTexture(ghostTex);
	ghostSprite.setScale(2,2);
	ghostSprite.setTextureRect(IntRect(0, 0, 32, 32));

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

	lvl[7][7] = '#';
	lvl[7][8] = '#';
	lvl[7][9] = '#';
	lvl[7][10] = '#';

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

		if (Keyboard::isKeyPressed(Keyboard::Left))
		{
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

		if (Keyboard::isKeyPressed(Keyboard::Right))
		{			
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
		//calling function to move ghosts
		moveGhosts(lvl, height, width, ghostX, ghostY, ghostDir, ghostSpeed, ghost_count,ghostTimer);


		window.clear();

		display_level(window, lvl, bgTex, bgSprite, blockTexture, blockSprite, height, width, cell_size);
		player_gravity(lvl,offset_y,velocityY,onGround,gravity,terminal_Velocity, player_x, player_y, cell_size, PlayerHeight, PlayerWidth);
		PlayerSprite.setPosition(player_x, player_y);
		window.draw(PlayerSprite);
		//rendering all ghosts
		for(int g = 0; g < ghost_count; g++)
		{
    		ghostSprite.setPosition(ghostX[g] * cell_size, ghostY[g] * cell_size);
    		window.draw(ghostSprite);
		}

		window.display();
		lvlMusic.stop();
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