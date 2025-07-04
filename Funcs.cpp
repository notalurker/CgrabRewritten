#include "Funcs.h"
#include <random>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "options-menu.h"

using namespace sf;
using namespace std;

void randomNumber()
{
    srand(static_cast<unsigned int>(time(nullptr)));
}

double getRandomNumber() {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen);
}

comicSlideShow::comicSlideShow()
{
    if(!frame1.loadFromFile("Sprites/comic/1.png") || !frame2.loadFromFile("Sprites/comic/2.png") || !frame3.loadFromFile("Sprites/comic/3.png"))
    {
        cout << "ERROR LOADING TEXTURE" << endl;
    }
}

void comicSlideShow::draw(RenderWindow* l)
{
    // Prepare slideshow data
    Texture* frames[] = { &frame1, &frame2, &frame3 };
    const int numFrames = 3;
    const float fadeInTime = 1.0f;
    const float holdTime = 5.0f;
    const float fadeOutTime = 1.0f;
    const float totalTime = fadeInTime + holdTime + fadeOutTime;

    int frameIndex = 0;
    bool running = true;
    Clock clock;

    // Create sprite for current frame
    if (currentframe) delete currentframe;
    currentframe = new Sprite(*frames[frameIndex]);

    while (l->isOpen() && running)
    {
        // Handle events (close window or skip with Space)
        while (const std::optional<sf::Event> event = l->pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                l->close();
                return;
            }
            if (const auto* pressedKey = event->getIf<sf::Event::KeyPressed>())
            {
                if (pressedKey->code == sf::Keyboard::Key::Space)
                {
                    running = false;
                }
            }
        }

        float elapsed = clock.getElapsedTime().asSeconds();
        float alpha = 0.f;

        if (elapsed < fadeInTime)
        {
            alpha = 255.f * (elapsed / fadeInTime);
        }
        else if (elapsed < fadeInTime + holdTime)
        {
            alpha = 255.f;
        }
        else if (elapsed < totalTime)
        {
            alpha = 255.f * (1.f - (elapsed - fadeInTime - holdTime) / fadeOutTime);
        }
        else
        {
            // Next frame or end
            frameIndex++;
            if (frameIndex >= numFrames)
            {
                running = false;
                continue;
            }
            clock.restart();
            if (currentframe) delete currentframe;
            currentframe = new Sprite(*frames[frameIndex]);
            continue;
        }

        currentframe->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(alpha)));

        l->clear();
        l->draw(*currentframe);
        l->display();
    }
}

gameEngine::gameEngine(RenderWindow* window)
    : window(window), player(window), state(GameState::Playing)
{
    currentCoinSpeed = 250.f;
    currentBombSpeed = 250.f;
    activeCoins = 1; //coins currently onscreen
    activeBombs = 0; //bombs currently onscreen
    activePowerups = 0;
    activePowerups = 0;
    bombsSlowed = false;
    slowBombTimer = 0.f;
    bombSlowFactor = 0.5f; // Factor to slow bombs

    coinThresholdTimer = 0.f;
    coinThresholdInterval = 15.f; // Interval for coin threshold in seconds
    bombThresholdTimer = 0.f;
    bombThresholdInterval = 25.f; // Interval for bomb threshold in seconds
    powerupThresholdTimer = 0.f;
    powerupThresholdInterval = 15.f; // Interval for powerup threshold in seconds

    if (!playBGM.openFromFile("Sprites/soundfx/playBGM.wav")) {
        cout << "ERROR LOADING BACKGROUND MUSIC" << std::endl;
    }
    else {
        //bgmMusic.setLoop(true);
        playBGM.setLooping(true);
        playBGM.setVolume(optionsMenu::musicVol); // Volume control
        playBGM.play();
    }
}

void gameEngine::clamp()
{
    if (activeCoins > MAX_COINS) activeCoins = MAX_COINS;
    if (activeBombs > MAX_BOMBS) activeBombs = MAX_BOMBS;
    if (activePowerups > MAX_POWERUPS) activePowerups = MAX_POWERUPS;
    if (activeBombs < 0) activeBombs = 0;
    if (activePowerups < 0) activePowerups = 0;
}

void gameEngine::spawncoins(float x)
{
    clamp();
    for (int i = 0; i < activeCoins; i++) { //falling coins
        coin[i].updatecoin(x);
        coin[i].rendercoin(window.window);
    }
}

void gameEngine::spawnbombs(float x)
{
    clamp();
    for (int i = 0; i < activeBombs && i < MAX_BOMBS; i++) { //falling bombs
        bomb[i].updatebomb(x);
        bomb[i].renderbomb(window.window);
    }
}

void gameEngine::spawnpowerups(float x)
{
    clamp();
    for (int i = 0; i < activePowerups; i++) { //falling powerup
        power[i].updatePowerup(x);
        power[i].renderPowerup(window.window);
    }
}

void gameEngine::collisionchecker()
{
    clamp();
    for (int i = 0; i < activeCoins; i++) { //collision checker for coins
        if (coin[i].spritecoin->getGlobalBounds().findIntersection(player.spriteplayer->getGlobalBounds())) {
            player.score += 100 * player.scoremultiplier;
            if (rand() % 10 + 1 == 1 && activeBombs < MAX_BOMBS)activeBombs += 1;
            coin[i].coinSounds->play();
            cout << "hit! " << player.score << endl;
            coin[i].respawncoin();
        }
    }
    for (int i = 0; i < activeBombs; i++) { //collision checker for bombs
        if (bomb[i].spritebomb->getGlobalBounds().findIntersection(player.spriteplayer->getGlobalBounds())) {
            player.health -= 1;
            bomb[i].bombSounds->play();
            cout << "explode! " << player.health << endl;
            bomb[i].respawnbomb(0);
        }
    }
    for (int i = 0; i < activePowerups; i++) { //collision checker for bombs
        if (power[i].randomPowerSprite->getGlobalBounds().findIntersection(player.spriteplayer->getGlobalBounds())) {
            power[i].powerupSounds->play();
            int chosen = rand() % 20 + 1;
            
            switch (chosen)
            {
            case 1:case 2:case 3:case 4: case 5:
                if (player.health < player.MAX_HEALTH)
                {
                    cout << "1 Health Added!" << endl;
                    status = "1 Health Added!";
                    player.health += 1;
                }
                else
                {
                    cout << "Max Health Reached!" << endl;
                    status = "Max Health Reached!";
                }
                break;
            case 6:
                cout << "Bombs are slowed!" << endl;
                status = "Bombs are slowed!";
                if (!bombsSlowed) {
                    bombsSlowed = true;
                    slowBombTimer = 2.f; // 2 seconds
                    for (int j = 0; j < activeBombs; ++j) {
                        bomb[j].bombAcceleration = bombSlowFactor;
                    }
                } else {
                    slowBombTimer = 2.f; // Reset timer if already slowed
                }
                break;
            case 7: case 8: case 9:
                status = "Increasing Player Speed...";
                player.moveSpeed += 25.f;
                break;
            case 10:
                status = "Removing Bombs...";
                for (int i = 0; i < activeBombs; ++i)
                {
                    float randomValue = PLAY_OFFSET_X + (PLAY_WIDTH - 64) * getRandomNumber();
                    bomb[i].spritebomb->setPosition({ randomValue, PLAY_OFFSET_Y });
                }
                activeBombs = 0;
                break;
            case 11:
                cout << player.scoremultiplier + 1 << " Multiplier! " << endl;
                status = to_string(player.scoremultiplier + 1) + " Multiplier!";
                player.scoremultiplier += 1;
                break;
            case 12:
                cout << "bombs are easier to dodge..." << endl;
                status = "bombs are easier to dodge...";
                for (int i = 0; i < activeBombs; i++)
                {
                    float newScaleX = bomb[i].spritebomb->getScale().x;
                    float newScaleY = bomb[i].spritebomb->getScale().y;

                    bomb[i].spritebomb->setScale({ newScaleX * .98f, newScaleY * .98f });
                }
                break;
            case 13:case 14: case 15: case 16: case 17:
                cout << "Unlucky! Added 1 Bomb" << endl;
                status = "Unlucky! Added 1 Bomb";
                activeBombs += 1;
                break;
            default:
                cout << "+500 Score!" << endl;
                player.score += 500;
                status = "+500 Score!"; // to be removed
                break;
            }
            for (int i = 0; i < activePowerups; ++i)
            {
                float randomValue = PLAY_OFFSET_X + (PLAY_WIDTH - 64) * getRandomNumber();
                power[i].randomPowerSprite->setPosition({ randomValue, PLAY_OFFSET_Y });
            }
            activePowerups--;
        }
    }

}

void gameEngine::thresholdchecker(float delta)
{
    clamp();

    // COINS
    coinThresholdTimer += delta;
    if (coinThresholdTimer >= coinThresholdInterval && activeCoins < MAX_COINS) {
        cout << "TIME THRESHOLD REACHED! Amount of Coins: " << activeCoins << endl;
        activeCoins++; 
    }
    if (coinThresholdTimer >= coinThresholdInterval)
    {
        currentCoinSpeed += 50.f;
        for (int i = 0; i < activeCoins; ++i) {
            coin[i].coinFallspeed = currentCoinSpeed;
        }
        coinThresholdTimer = 0.f;
    }

    // BOMBS
    bombThresholdTimer += delta;
    int oldActiveBombs = activeBombs;
    if (bombThresholdTimer >= bombThresholdInterval && activeBombs < MAX_BOMBS) {
        cout << "TIME THRESHOLD REACHED! Amount of Bombs: " << activeBombs << endl;
        activeBombs++;
    }
    if (bombThresholdTimer >= bombThresholdInterval) {
        currentBombSpeed += 50.f;
        for (int i = 0; i < activeBombs; ++i) {
            bomb[i].bombFallspeed = currentBombSpeed;
            if (i >= oldActiveBombs) {
                bomb[i].respawnbomb(0);
            }
        }
        bombThresholdTimer = 0.f;
    }

    // POWERUPS (unchanged)
    powerupThresholdTimer += delta;
    if (powerupThresholdTimer >= powerupThresholdInterval && activePowerups < MAX_POWERUPS) {
        cout << "TIME THRESHOLD REACHED! Amount of Powerups: " << activePowerups << endl;
        activePowerups++;
    }
    if (powerupThresholdTimer >= powerupThresholdInterval) {
        for (int i = 0; i < activePowerups; ++i) {
            power[i].powerupFallspeed += 50.f;
        }
        powerupThresholdTimer = 0.f;
    }
}

void gameEngine::bombSlowchecker(float x)
{
    if (bombsSlowed) {
        slowBombTimer -= x;
        if (slowBombTimer <= 0.f) {
            bombsSlowed = false;
            for (int i = 0; i < activeBombs; ++i) {
                bomb[i].bombAcceleration = 1.f; // Reset bomb acceleration
            }
        }
    }
}

void gameEngine::updatetext()
{
    window.T_score->setString("Score: " + to_string(player.score));
    sf::FloatRect scoreBounds = window.T_score->getLocalBounds();
    window.T_score->setOrigin({ scoreBounds.size.x / 2.f, scoreBounds.size.y / 2.f });
    window.T_score->setPosition(Vector2f(WINDOW_WIDTH / 2, 30.f));
    window.T_score->setCharacterSize(64);
    window.T_score->setFillColor(sf::Color::White);

    window.T_status->setString(status);
    sf::FloatRect statusBounds = window.T_status->getLocalBounds();
	window.T_status->setOrigin({ statusBounds.size.x / 2.f, statusBounds.size.y / 2.f });
	window.T_status->setPosition(Vector2f(WINDOW_WIDTH / 2, 1020.f));
	window.T_status->setCharacterSize(45);
	window.T_status->setFillColor(sf::Color::White);

    window.window->draw(*window.T_score);
    window.window->draw(*window.T_status);
}

void gameEngine::run()
{
    clamp();
    Clock clock;
    bool goToMenu = false;
    while (window.window->isOpen()) {
        float delta = clock.restart().asSeconds();
        window.window->clear();
        window.window->draw(*window.spritebg);
        //draw hearts

        if (state == GameState::Playing) {
            updatetext();
            player.checkEvent(window.window, delta);
            player.renderplayer(window.window);
            if (activeCoins)spawncoins(delta);
            if (activeBombs)spawnbombs(delta);
            if (activePowerups)spawnpowerups(delta);
            collisionchecker();
            thresholdchecker(delta);
            bombSlowchecker(delta);

            if (player.health <= 0 || Keyboard::isKeyPressed(Keyboard::Key::Backspace)) {

                state = GameState::GameOver;
				playBGM.stop(); // Stop background music on game over
				gameover.loseSound->play(); // Play lose sound
            }

        } 
        else if (state == GameState::GameOver) {
            gameover.endscore->setString("Score: " + to_string(player.score));
            gameover.draw(window.window);
            gameover.checkEvent(window.window, this, &goToMenu);
            if (goToMenu) break;
        }

        window.window->display();
    }
}

void gameEngine::reset()
{
    // Reset player
    player.score = 0;
    player.scoremultiplier = 1;
    player.health = 5;
    player.moveSpeed = 700.f;
    player.playerX = WINDOW_WIDTH / 2; //init for startpos
    player.spriteplayer->setPosition({ player.playerX, PLAY_OFFSET_Y + PLAY_HEIGHT - player.spriteplayer->getGlobalBounds().size.y });
    activeCoins = 1;
    activeBombs = 0;
    activePowerups = 0;
    bombsSlowed = false;
    slowBombTimer = 0.f;
    bombSlowFactor = 0.5f;
    status = "";
    currentBombSpeed = 250.f;
    currentCoinSpeed = 250.f;

    coinThresholdTimer = 0.f;
    bombThresholdTimer = 0.f;
    powerupThresholdTimer = 0.f;

    for (int i = 0; i < MAX_COINS; ++i) coin[i].coinFallspeed = 250.f;
    for (int i = 0; i < MAX_BOMBS; ++i) bomb[i].bombFallspeed = 250.f;
    for (int i = 0; i < MAX_POWERUPS; ++i) power[i].powerupFallspeed = 250.f;

    // Respawn all objects
    for (int i = 0; i < MAX_COINS; ++i) coin[i].respawncoin();
    for (int i = 0; i < MAX_BOMBS; ++i) bomb[i].respawnbomb(0);
    for (int i = 0; i < MAX_POWERUPS; ++i) power[i].respawnPowerup();

    // Reset state
    state = GameState::Playing;
	playBGM.play(); // Restart background music
	playBGM.setLooping(true); // Music loops
}

gameWindow::gameWindow(RenderWindow* window) : window(window) {
    if (!gameFont.openFromFile("Sprites/Fonts/ka1.ttf")) {
        cerr << "ERROR LOADING FONT" << endl;
    }
    T_score = new Text(gameFont);
    T_status = new Text(gameFont);

    T_score->setCharacterSize(32);
    T_status->setCharacterSize(24);
    

    T_score->setFillColor({ 0, 0, 0 });
    T_status->setFillColor({ 0, 0, 0 });
    
    if (!texturebg.loadFromFile("Sprites/bg/testbg.png")) {
        cerr << "ERROR LOADING SPRITE" << endl;
    }
    spritebg = new Sprite(texturebg);
    spritebg->setPosition({ 0.f, 0.f });
}

gameWindow::~gameWindow() //deconstructor
{
    
    delete spritebg;
    delete T_status;
    delete T_score;
}

Player::~Player() //deconstructor
{
    delete spriteplayer;
    delete spriteheart;
}
 
Player::Player(RenderWindow* l)
{
    scoremultiplier = 1;
    score = 0;
    health = 5;
    if (!textureplayer.loadFromFile("Sprites/player/player.png")) //checks if it load properly
    {
        cout << "ERROR LOADING SPRITE" << endl;
    }
    if (!textureheart.loadFromFile("Sprites/hearts/hearts.png")) //checks if it load properly
    {
        cout << "ERROR LOADING SPRITE" << endl;
    }
    spriteheart = new Sprite(textureheart);
    spriteheart->setTextureRect(IntRect({ 0, 0 }, { 32, 32 }));
    spriteheart->setScale({ 2.5f, 2.5f });

    spriteplayer = new Sprite(textureplayer); //init for player sprite
    spriteplayer->setTextureRect(IntRect({ 0, 0 }, { 64, 64 })); //sets the sprite as a rectangle
	spriteplayer->setOrigin({ 32.f, 0.f }); //sets the origin to the center of the sprite
    spriteplayer->setScale({ 2.5f, 2.5f }); //triples the size
    moveSpeed = 700.f; //init for player ms
    frameDuration = 0.15f;
    frameTimer = 0.f;
    frameWidth = 64;
    frameHeight = 64;
    totalFrames = 4;
    currentFrame = 0;
    
    playerX = WINDOW_WIDTH / 2; //init for startpos
    spriteplayer->setPosition({ playerX, PLAY_OFFSET_Y + PLAY_HEIGHT - spriteplayer->getGlobalBounds().size.y });
    


}

void Player::renderplayer(RenderWindow* l) //draws the player
{
    for (int i = 0; i < MAX_HEALTH; i++)
    {
        spriteheart->setPosition({ 1760, 330 + (32.f * i * 3.f) });  // adjust spacing as needed

        if (health > i)
        {
            spriteheart->setTextureRect(IntRect({ 0, 0 }, { 32, 32 }));
        }
        else
        {
            spriteheart->setTextureRect(IntRect({ 32, 0 }, { 32, 32 }));
        }
        l->draw(*spriteheart);
    }
    l->draw(*spriteplayer); 
}

void Player::checkEvent(RenderWindow* l, float x)
{
    while (auto event = l->pollEvent()) //checks for event
    {
        
        if (event->is<Event::Closed>()) //when x is clicked
        {
            l->close();
        }
    }

    if (Keyboard::isKeyPressed(Keyboard::Key::D) || Keyboard::isKeyPressed(Keyboard::Key::Right))
    {
        moveRight(x);
    }
    if (Keyboard::isKeyPressed(Keyboard::Key::A) || Keyboard::isKeyPressed(Keyboard::Key::Left))
    {
        moveLeft(x);
    }
}

void Player::moveRight(float x)
{
    spriteplayer->setScale({ -2.5f, 2.5f });
    playerX += moveSpeed * x;
    float spriteWidth = spriteplayer->getGlobalBounds().size.x;
    float minX = PLAY_OFFSET_X;
    float maxX = PLAY_OFFSET_X + PLAY_WIDTH - (spriteWidth/2) ;
    if (playerX > maxX) {
        playerX = maxX;
    }

    updateplayer(x);

    spriteplayer->setPosition({ playerX, PLAY_OFFSET_Y + PLAY_HEIGHT - spriteplayer->getGlobalBounds().size.y });
}

void Player::moveLeft(float x)
{
    spriteplayer->setScale({ 2.5f, 2.5f });
    playerX -= moveSpeed * x;

    float spriteWidth = spriteplayer->getGlobalBounds().size.x;
    float minX = PLAY_OFFSET_X + (spriteWidth/2);
    if (playerX < minX){
        playerX = minX;
    }

    updateplayer(x);

    spriteplayer->setPosition({ playerX, PLAY_OFFSET_Y + PLAY_HEIGHT - spriteplayer->getGlobalBounds().size.y });
}

void Player::updateplayer(float deltaTime) {
	frameTimer += deltaTime;
	if (frameTimer >= frameDuration) {
		currentFrame = (currentFrame + 1) % totalFrames;
		spriteplayer->setTextureRect(IntRect({ currentFrame * frameWidth, 0 }, { frameWidth, frameHeight }));
		frameTimer = 0.f;
	}
}

Money::Money()
{

    if (!takeCoin.loadFromFile("Sprites/soundfx/pickupCoin.wav")) {
        cout << "ERROR LOADING COIN SOUND" << endl;
    }
    coinSounds = new Sound(takeCoin);
    coinSounds->setBuffer(takeCoin);
    coinSounds->setVolume(optionsMenu::soundFX);
    coinFallspeed = 250.f;
    frameDuration = 0.15f;
    frameTimer = 0.f;
    frameWidth = 32;
    frameHeight = 32;
    totalFrames = 8;
    currentFrame = 0;
    if (!texturecoin.loadFromFile("Sprites/money/coin.png"))
    {
        cout << "ERROR LOADING SPRITE" << endl;
    }
    spritecoin = new Sprite(texturecoin);
    spritecoin->setTextureRect(IntRect({ 0, 0 }, { 32, 32 }));
    spritecoin->setScale({ 2.f, 2.f });
    respawncoin();
    void respawncoinInWindow(float);
    void respawnbombInWindow(float);
    void respawnPowerupInWindow(float);
}

void Money::respawncoin() {
    randomValX = PLAY_OFFSET_X + (PLAY_WIDTH - 80) * getRandomNumber();
    float y = PLAY_OFFSET_Y;
    spritecoin->setPosition({ randomValX, y });
    fallSpeed = coinFallspeed + getRandomNumber() * 100.f;
}

void Money::updatecoin(float deltaTime) {
    frameTimer += deltaTime;

    if (frameTimer >= frameDuration) {
        currentFrame = (currentFrame + 1) % totalFrames;
        spritecoin->setTextureRect(IntRect({ currentFrame * frameWidth, 0 }, { frameWidth, frameHeight }));
        frameTimer = 0.f;
    }

    auto Xpos = spritecoin->getPosition().x;
    auto Ypos = spritecoin->getPosition().y;
    spritecoin->setPosition({ Xpos, Ypos + fallSpeed * deltaTime });

    if (spritecoin->getPosition().y > PLAY_OFFSET_Y + PLAY_HEIGHT - spritecoin->getGlobalBounds().size.y) {
        respawncoin();
    }
}

void Money::rendercoin(RenderWindow* l)
{
    l->draw(*spritecoin);
}

Bomb::Bomb()
{
    if (!takeBomb.loadFromFile("Sprites/soundfx/explosion.wav")) {
        cout << "ERROR LOADING COIN SOUND" << endl;
    }
    bombSounds = new Sound(takeBomb);
    bombSounds->setBuffer(takeBomb);
    bombSounds->setVolume(optionsMenu::soundFX);
    bombFallspeed = 250.f;
    bombAcceleration = 1.f;
    frameDuration = 0.4f;
    frameTimer = 0.f;
    frameWidth = 32;
    frameHeight = 32;
    totalFrames = 2;
    currentFrame = 0;
    if (!texturebomb.loadFromFile("Sprites/bomb/bomb.png"))
    {
        cout << "ERROR LOADING SPRITE" << endl;
    }
    spritebomb = new Sprite(texturebomb);
    spritebomb->setTextureRect(IntRect({ 0, 0 }, { 32, 32 }));
    spritebomb->setScale({ 2.f, 2.f });
    respawnbomb(0);
}

void Bomb::respawnbomb(float y) {
    randomValX = PLAY_OFFSET_X + (PLAY_WIDTH - 64) * getRandomNumber();
    spritebomb->setPosition({ randomValX, PLAY_OFFSET_Y + y });
    fallSpeed = bombFallspeed + getRandomNumber() * 100.f * bombAcceleration;
}

void Bomb::updatebomb(float x) {
    frameTimer += x;

    if (frameTimer >= frameDuration) {
        currentFrame = (currentFrame + 1) % totalFrames;
        spritebomb->setTextureRect(IntRect({ currentFrame * frameWidth, 0 }, { frameWidth, frameHeight }));
        frameTimer = 0.f;
    }

    auto Xpos = spritebomb->getPosition().x;
    auto Ypos = spritebomb->getPosition().y;
    spritebomb->setPosition({ Xpos, Ypos + fallSpeed * x * bombAcceleration });

    if (spritebomb->getPosition().y > PLAY_OFFSET_Y + PLAY_HEIGHT - spritebomb->getGlobalBounds().size.y) {
        respawnbomb(0);
    }
}

void Bomb::renderbomb(RenderWindow* l)
{
    l->draw(*spritebomb);
}

Powerups::Powerups()
{
    powerupFallspeed = 250.f;
    if (!PowerTexture.loadFromFile("Sprites/powerups/powerups.png")) //checks if it load properly
    {
        cout << "ERROR LOADING SPRITE" << endl;
    }

    frameDurations = { 1.f, 0.1f, 0.2f, 0.1f };
    frameTimer = 0.f;
    frameWidth = 32;
    frameHeight = 32;
    totalFrames = 4;
    currentFrame = 0;

    randomPowerSprite = new Sprite(PowerTexture);
    randomPowerSprite->setTextureRect(IntRect({ 0, 0 }, { 32, 32 }));
    randomPowerSprite->setScale({ 2.f, 2.f });
    respawnPowerup();

	// Powerup SFX
    if (!powerupBuffer.loadFromFile("Sprites/soundfx/powerUP.wav")) {
        cout << "ERROR LOADING POWERUP SOUND" << endl;
    }
    powerupSounds = new Sound(powerupBuffer);
    powerupSounds->setBuffer(powerupBuffer);
    powerupSounds->setVolume(optionsMenu::soundFX); // Adjust as needed
}

void Powerups::respawnPowerup()
{
    randomValue = PLAY_OFFSET_X + (PLAY_WIDTH - 64) * getRandomNumber();
    float y = PLAY_OFFSET_Y;
    randomPowerSprite->setPosition({ randomValue, y });
    fallSpeed = powerupFallspeed + getRandomNumber() * 100.f;
}

void Powerups::updatePowerup(float y)
{
    auto Xpos = randomPowerSprite->getPosition().x;
    auto Ypos = randomPowerSprite->getPosition().y;
    randomPowerSprite->setPosition({ Xpos, Ypos + fallSpeed * y });

    frameTimer += y;

	if (frameTimer >= frameDurations[currentFrame]) {
		currentFrame = (currentFrame + 1) % totalFrames;
		randomPowerSprite->setTextureRect(IntRect({ currentFrame * frameWidth, 0 }, { frameWidth, frameHeight }));
		frameTimer = 0.f;
	}

    if (randomPowerSprite->getPosition().y > PLAY_OFFSET_Y + PLAY_HEIGHT - randomPowerSprite->getGlobalBounds().size.y && rand() % 8 + 1 >= 4) {
        respawnPowerup();
    }

}

void Powerups::renderPowerup(RenderWindow* l)
{
    l->draw(*randomPowerSprite);
}

gameOver::gameOver()
{
    if (!textureRetryButton.loadFromFile("Sprites/buttons/restartbutton.png")) //checks if it load properly
    {
        cout << "ERROR LOADING SPRITE" << endl;
    }
    if (!textureQuitButton.loadFromFile("Sprites/buttons/menubutton.png")) //checks if it load properly
    {
        cout << "ERROR LOADING SPRITE" << endl;
    }
    if (!textureOverTray.loadFromFile("Sprites/buttons/tray.png")) //checks if it load properly
    {
        cout << "ERROR LOADING SPRITE" << endl;
    }
    if (!gameoverFont.openFromFile("Sprites/Fonts/ka1.ttf"))
    {
        cout << "ERROR LOADING FONT" << endl;
    }
    
    // GAMEOVER SFX
    if (!loseBuffer.loadFromFile("Sprites/soundfx/lose.wav")) {
        std::cout << "ERROR LOADING LOSE SOUND" << std::endl;
    }
    loseSound = new Sound(loseBuffer);
    loseSound->setBuffer(loseBuffer);
    loseSound->setVolume(optionsMenu::soundFX); // Adjust as needed

	// HOVER SFX
    if (!hoverBuffer.loadFromFile("Sprites/soundfx/hover.wav")) {
        std::cout << "ERROR LOADING HOVER SOUND" << std::endl;
    }
    hoverSound = new Sound(hoverBuffer);
    hoverSound->setBuffer(hoverBuffer);
    hoverSound->setVolume(optionsMenu::soundFX);

    wasOverRetry = false;
    wasOverQuit = false;

    // CLICK SFX
    if (!clickBuffer.loadFromFile("Sprites/soundfx/click.wav")) {
        std::cout << "ERROR LOADING CLICK SOUND" << std::endl;
    }
    clickSound = new Sound(clickBuffer);
    clickSound->setBuffer(clickBuffer);
    clickSound->setVolume(optionsMenu::soundFX); // Adjust as needed


    float trayWidth = 750.f;
    float trayHeight = 450.f;
    float trayX = (1920.f - trayWidth) / 2.f;
    float trayY = (1080.f - trayHeight) / 2.f;
    spriteOverTray = new Sprite(textureOverTray);
    spriteOverTray->setPosition({ trayX, trayY });
    
    spriteRetryButton = new Sprite(textureRetryButton);
    spriteQuitButton = new Sprite(textureQuitButton);

    float buttonScale = 0.75f;

    float retryWidth = spriteRetryButton->getLocalBounds().size.x * buttonScale;
    float retryHeight = spriteRetryButton->getLocalBounds().size.y * buttonScale;
    float quitWidth = spriteQuitButton->getLocalBounds().size.x * buttonScale;
    float quitHeight = spriteQuitButton->getLocalBounds().size.y * buttonScale;

    float sideMargin = 100.f;
    float bottomMargin = 80.f;

    spriteRetryButton->setPosition({trayX + sideMargin, trayY + trayHeight - retryHeight - bottomMargin});
    spriteQuitButton->setPosition({ trayX + trayWidth - quitWidth - sideMargin, trayY + trayHeight - quitHeight - bottomMargin });


    spriteRetryButton->setScale({ buttonScale, buttonScale });
    spriteQuitButton->setScale({ buttonScale, buttonScale });
    

    // Store initial scales for hover effect
    retryButtonInitialScale = spriteRetryButton->getScale();
    quitButtonInitialScale = spriteQuitButton->getScale();

    lostHeader = new Text(gameoverFont);
    lostHeader->setCharacterSize(102);
    lostHeader->setFillColor(Color::White);
    lostHeader->setString("GAME OVER!");

    endscore = new Text(gameoverFont);
    endscore->setCharacterSize(64);
    endscore->setFillColor(Color::White);

    float headerX = trayX + 93.f;
    float headerY = trayY + 65.f;
    lostHeader->setPosition({ headerX, headerY });
}

void gameOver::draw(RenderWindow* l)
{   
    l->draw(*spriteOverTray);
    l->draw(*lostHeader);
    float scoreX = ((1920.f - 750) / 2.f) + 215.f;
    float scoreY = ((1080.f - 450) / 2.f) + 180.f;
    endscore->setPosition({ scoreX, scoreY });
    endscore->setFillColor(Color::Yellow);
    l->draw(*endscore);
    l->draw(*spriteQuitButton);
    l->draw(*spriteRetryButton);
}

void gameOver::checkEvent(RenderWindow* l, gameEngine* engine, bool* goToMenu)
{
    while (auto event = l->pollEvent())
    {
        if (event->is<Event::Closed>())
        {
            l->close();
        }
        if (event->is<Event::MouseButtonPressed>())
        {
            auto mouseClick = Vector2f(Mouse::getPosition(*l));
            if (spriteRetryButton->getGlobalBounds().contains(mouseClick))
            {
                clickSound->play();
                engine->reset();
            }
            if (spriteQuitButton->getGlobalBounds().contains(mouseClick))
            {
                clickSound->play();
                if (goToMenu) *goToMenu = true;
            }
        }
    }

    Vector2f mousePos = Vector2f(Mouse::getPosition(*l));
    Color buttonHighlight(200, 200, 200);
    Color buttonNormal(255, 255, 255, 255);
    const float hoverScaleFactor = 1.1f;

    bool overRetry = spriteRetryButton->getGlobalBounds().contains(mousePos);
    bool overQuit = spriteQuitButton->getGlobalBounds().contains(mousePos);

    if ((overRetry && !wasOverRetry) || (overQuit && !wasOverQuit)) {
        hoverSound->play();
    }

    if (overRetry) {
        spriteRetryButton->setScale(retryButtonInitialScale * hoverScaleFactor);
        spriteRetryButton->setColor(buttonHighlight);
    } else {
        spriteRetryButton->setScale(retryButtonInitialScale);
        spriteRetryButton->setColor(buttonNormal);
    }

    if (overQuit) {
        spriteQuitButton->setScale(quitButtonInitialScale * hoverScaleFactor);
        spriteQuitButton->setColor(buttonHighlight);
    } else {
        spriteQuitButton->setScale(quitButtonInitialScale);
        spriteQuitButton->setColor(buttonNormal);
    }

    wasOverRetry = overRetry;
    wasOverQuit = overQuit;
}

