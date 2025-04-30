#include <SFML/Graphics.hpp>
#include "FastNoiseLite.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <random> 

const int TILE_WIDTH = 32;
const int TILE_HEIGHT = 32;
const int MAP_WIDTH = 300;
const int MAP_HEIGHT = 300;
const int MAP_DEPTH = 20;

const float ZOOM_SPEED = 0.1f;
const float GRAVITY = 0.2f;
const float JUMP_STRENGTH = 5.0f;

// Tableau 3D des collisions
bool collisionMap[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH] = { false };

// Fonction pour vérifier si un mouvement est possible
bool canMoveTo(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT || z >= MAP_DEPTH)
        return false;
    return !collisionMap[x][y][z];
}

int main() {
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Carte Isométrique - Verdaterra");
    window.setFramerateLimit(60);

    sf::View view(sf::FloatRect(0, 0, 1280, 720));
    float zoomLevel = 1.0f;

    sf::Texture tileset;
    if (!tileset.loadFromFile("IsoTiles.png")) {
        std::cerr << "Erreur : Impossible de charger IsoTiles.png\n";
        return -1;
    }

    sf::Sprite tile(tileset);
    tile.setTextureRect(sf::IntRect(32, 0, TILE_WIDTH, TILE_HEIGHT));

    std::random_device rd;
    std::mt19937 gen(rd()); // Générateur Mersenne Twister

    // Distribution uniforme entre 1 et 100
    std::uniform_int_distribution<> distrib(1, 100);

    // Générer un nombre
    int random_number = distrib(gen);

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic);
    noise.SetSeed(random_number);
    noise.SetFrequency(0.08f);

    std::vector<std::vector<int>> heightMap(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, 0));

    // Génération de la carte + collision map
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            float noiseValue = noise.GetNoise((float)x, (float)y);
            int height = static_cast<int>((noiseValue + 1.0f) * 10);
            heightMap[y][x] = height;
            collisionMap[x][y][height] = true;
        }
    }

    // Joueur
    sf::Vector2f playerPos(0, 0);
    sf::Vector2f playerVelocity(0.f, 0.f);
    const float playerSpeed = 5.0f;

    sf::Texture playerTexture;
    if (!playerTexture.loadFromFile("player.png")) {
        std::cerr << "Erreur : Impossible de charger PlayerSprite.png\n";
        return -1;
    }
    sf::Sprite playerSprite(playerTexture);
    playerSprite.setTextureRect(sf::IntRect(0, 0, 32, 64));
    playerSprite.setOrigin(0, 54); // Centré bas du sprite

    static sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.delta > 0) { 
                    if(zoomLevel <= 2.0f && zoomLevel >= 0.5f){
                        zoomLevel -= ZOOM_SPEED;
                    } 
                } else { 
                    if(zoomLevel <= 2.0f && zoomLevel >= 0.5f){
                        zoomLevel += ZOOM_SPEED;
                    }
                } 
                view.setSize(1280 * zoomLevel, 720 * zoomLevel);
            }
        }
        std::cout << "Zoom level: " << zoomLevel << std::endl;

        // Calcul du deltaTime
        float deltaTime = clock.restart().asSeconds();

        // Initialisation de la vitesse
        playerVelocity = {0.f, 0.f};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            playerVelocity.x -= playerSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            playerVelocity.x += playerSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            playerVelocity.y -= playerSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            playerVelocity.y += playerSpeed;
        }

        // Application du mouvement
        playerPos += playerVelocity * deltaTime;

        // Clamp pour éviter de sortir de la carte
        if (playerPos.x < 0) playerPos.x = 0;
        if (playerPos.y < 0) playerPos.y = 0;
        if (playerPos.x >= MAP_WIDTH) playerPos.x = MAP_WIDTH - 1;
        if (playerPos.y >= MAP_HEIGHT) playerPos.y = MAP_HEIGHT - 1;

        float playerZ = heightMap[static_cast<int>(playerPos.y)][static_cast<int>(playerPos.x)];
        std::cout << "Position joueur: (" << playerPos.x << ", " << playerPos.y << ", " << playerZ << ")\n";

        window.clear(sf::Color::Black);
        window.setView(view);

        // Rendu des tuiles avec ombrage selon la hauteur
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                int tileHeight = heightMap[y][x];
                int screenX = (x - y) * (TILE_WIDTH / 2) + 640;
                int screenY = (x + y) * (TILE_HEIGHT / 4) + 360 - tileHeight * TILE_HEIGHT / 2;

                tile.setPosition(screenX, screenY);

                // Ombre basée sur la hauteur
                int shade = 100 + tileHeight * 10;
                shade = std::max(100, std::min(255, shade));
                tile.setColor(sf::Color(shade, shade, shade));

                if(tileHeight > 12){
                    tile.setTextureRect(sf::IntRect(32, 0, TILE_WIDTH, TILE_HEIGHT));
                } if(tileHeight < 5){
                    tile.setTextureRect(sf::IntRect(64, 0, TILE_WIDTH, TILE_HEIGHT));
                } else {
                    tile.setTextureRect(sf::IntRect(0, 0, TILE_WIDTH, TILE_HEIGHT));
                }  
                window.draw(tile);
            }
        }

        // Affichage du joueur
        int playerScreenX = (playerPos.x - playerPos.y) * (TILE_WIDTH / 2) + 640;
        int playerScreenY = (playerPos.x + playerPos.y) * (TILE_HEIGHT / 4) + 360 - static_cast<int>(playerZ) * TILE_HEIGHT / 2;

        playerSprite.setPosition(playerScreenX, playerScreenY);
        view.setCenter(playerScreenX+16, playerScreenY-32);
        window.draw(playerSprite);

        window.display();
    }

    return 0;
}
