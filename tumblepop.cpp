#include <iostream>
#include <fstream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>

using namespace sf;
using namespace std;

int screen_x = 1136;
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
	if(onGround){
		velocityY = jumpStrength;
		onGround = false;
	}
}

void ghost_move(float& ghost_x, float& ghost_y, int& ghost_direction, float ghost_speed, char** lvl, int cell_size){
	int posiY=ghost_y/cell_size;
	int posiX=ghost_x/cell_size;
	int newX=(ghost_x + ghost_speed)*ghost_direction;
	if(lvl[posiX][posiY]== '#')
		ghost_direction = -ghost_direction;
	
}

void player_gravity(char** lvl, float& offset_y, float& velocityY, bool& onGround, const float& gravity, float& terminal_Velocity, float& player_x, float& player_y, const int cell_size, int& Pheight, int& Pwidth)
{
    // Store the original Y position
    float original_y = player_y;

    offset_y = player_y;
    offset_y += velocityY; 

    // 1. Check potential collision tiles at the *new* offset position
    char bottom_left_down = lvl[(int)(offset_y + Pheight) / cell_size][(int)(player_x ) / cell_size];
    char bottom_right_down = lvl[(int)(offset_y  + Pheight) / cell_size][(int)(player_x + Pwidth) / cell_size];
    char bottom_mid_down = lvl[(int)(offset_y + Pheight) / cell_size][(int)(player_x + Pwidth / 2) / cell_size];

    bool blockBelow = (bottom_left_down == '#' || bottom_mid_down == '#' || bottom_right_down == '#');
    bool shouldLand = false;

    // Condition to Land: Must have a block below AND be moving downwards
    if (blockBelow && velocityY > 0)
    {
        // Calculate the Y coordinate of the block's top edge
        int block_y_index = (int)(offset_y + Pheight) / cell_size; 
        float block_top_y = (float)block_y_index * cell_size;
        
        // Check if the player's bottom edge (at the *original* position) was above the block's top edge
        // This confirms the player is landing ON the block, not hitting its side.
        if (original_y + Pheight <= block_top_y)
        {
            shouldLand = true;
        }
    }

    if (shouldLand)
    {
        onGround = true;
        velocityY = 0;
        
        // SNAP: Calculate the block index and snap to the top, offset by -1 pixel (Epsilon)
        int block_y_index = (int)(offset_y + Pheight) / cell_size;
        player_y = block_y_index * cell_size - Pheight - 1.0f; 
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
	float player_x = 500;
	float player_y = 150;

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
	float ghost_x=150,ghost_y=70,ghost_speed=2;
	int ghost_direction;

	Texture ghostTex;
	Sprite ghostSprite;

	ghostTex.loadFromFile("Data/ghost.png");
	ghostSprite.setTexture(ghostTex);
	ghostSprite.setScale(3,3);
	ghostSprite.setPosition(ghost_x,ghost_y);
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

		if (Keyboard::isKeyPressed(Keyboard::Right))
		{			
			right_top = lvl[(int)(player_y+PlayerHeight) / cell_size][(int)(player_x + PlayerWidth) / cell_size];
			right_mid = lvl[(int)(player_y+PlayerHeight/2) / cell_size][(int)(player_x + PlayerWidth) / cell_size];
			right_bottom = lvl[(int)(player_y) / cell_size][(int)(player_x + PlayerWidth) / cell_size];
			if(right_mid != '#' && right_bottom != '#' && right_top != '#')
				player_x += speed;
			else;
				right_top = right_bottom = right_mid = '\0';
		}

		if (Keyboard::isKeyPressed(Keyboard::Left))
		{
			left_top = lvl[(int)(player_y+PlayerHeight) / cell_size][(int)(player_x) / cell_size];
			left_mid = lvl[(int)(player_y+PlayerHeight/2) / cell_size][(int)(player_x) / cell_size];
			left_bottom = lvl[(int)(player_y) / cell_size][(int)(player_x) / cell_size];
				if(left_mid != '#' && left_bottom != '\0' && left_top != '#')
					player_x -= speed;
				else
					left_top = left_bottom = left_mid = '\0';;
				
		}
		if (Keyboard::isKeyPressed(Keyboard::Up))
		{
			isJumping = true;
			jump(onGround, velocityY, jumpStrength);
		}
		if (Keyboard::isKeyPressed(Keyboard::Down))
		{
			player_y+=20;
		}
		
		if(player_x <= cell_size)
			player_x = cell_size;



		window.clear();

		display_level(window, lvl, bgTex, bgSprite, blockTexture, blockSprite, height, width, cell_size);
		player_gravity(lvl,offset_y,velocityY,onGround,gravity,terminal_Velocity, player_x, player_y, cell_size, PlayerHeight, PlayerWidth);
		PlayerSprite.setPosition(player_x, player_y);
		window.draw(PlayerSprite);
		window.draw(ghostSprite);

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


