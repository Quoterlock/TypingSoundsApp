#include <SFML/Audio.hpp>
#include <iostream>
#include <regex>
#include <fstream>
#include <string>
#include <Windows.h>
#include <filesystem>
#include <conio.h>
#include <chrono>

#define SOUNDS_COUNT 16
#define BUFFERS_COUNT 7 // enter + backspace + space + [...keys sounds]

struct Config {
    std::string soundsFolder;
    int volume;
    int volumeMaxDelta;
    float pitchMaxDelta;
};

std::chrono::high_resolution_clock::time_point prevClickTime;
int prevCharacter;

Config config;
sf::Sound sounds[SOUNDS_COUNT];
sf::SoundBuffer buffers[BUFFERS_COUNT];

bool isExists(std::string filename) {
    return std::filesystem::exists(filename);
}

void checkConfigFile() {
    if (!isExists("config.txt")) {
        std::ofstream outFile("config.txt");
        if (outFile.is_open()) {
            outFile << "selected-preset : default\n"
                << "volume(%) : 50\n" << "pitch_max_delta : 0.01\n"
                << "volume_max_delta : 5\n";
            outFile.close();
            std::cout << "Config file created successfully.\n";
        }
        else {
            std::cerr << "Error: Unable to create config file.\n";
        }
    }
}

int getConfig() {
    checkConfigFile();

    // set defalut values
    config.soundsFolder = "sounds\\default\\";
    config.volume = 50;
    config.volumeMaxDelta = 5;
    config.pitchMaxDelta = 0.01;
    
    // read from config.txt file
    std::regex pattern(R"((.+?) : (.+))");
    std::ifstream file("config.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open config.txt file." << std::endl;
        return 1;
    }

    // read line by line
    std::string input;
    while (std::getline(file, input)) {
        std::smatch matches;
        if (std::regex_match(input, matches, pattern)) {
            std::string property = matches[1].str();
            std::string value = matches[2].str();

            // define config values
            if (property == "selected-preset") {
                config.soundsFolder = "sounds\\" + value + "\\";
                std::cout << "Selected Preset is \"" << value << "\".\n";
            }
            if (property == "volume(%)") {
                config.volume = std::stoi(value);
                std::cout << "Volume is " << value << "%\n";
            }
            if (property == "pitch_max_delta") {
                config.pitchMaxDelta = std::stof(value);
            }
            if (property == "volume_max_delta") {
                config.volumeMaxDelta = std::stoi(value);
            }
        }
    }
    file.close();
    return 0;
}

int initSounds() {

    // init buffers
    if (buffers[0].loadFromFile(config.soundsFolder + "enter_click.wav")) {
        std::cout << "enter_click.wav loaded...\n";
    }
    if (buffers[1].loadFromFile(config.soundsFolder + "backspace_click.wav")) {
        std::cout << "backspace_click.wav loaded...\n";
    }
    if (buffers[2].loadFromFile(config.soundsFolder + "space_click.wav")) {
        std::cout << "space_click.wav loaded...\n";
    }
    for (int i = 3; i < BUFFERS_COUNT; i++) {
        if (buffers[i].loadFromFile(config.soundsFolder + "key_click_" + std::to_string(i - 3) + ".wav")) {
            std::cout << "key_click_" + std::to_string(i - 3) + ".wav loaded...\n";
        }
    }

    // init sounds
    for (int i = 0; i < SOUNDS_COUNT; i++) {
        sounds[i].setVolume((float)config.volume);
        sounds[i].setPosition(0.0f, 0.0f, 0.0f);
    }
    return 0;
}

int selectSound(int key) {
    if (key == 32) { // space
        return 2;
    }
    else if (key == 8) { // backspace
        return 1;
    }
    else if (key == 13) { // enter
        return 0;
    }
    else {
        // select random number from 3 to N
        return 3 + std::rand() % (BUFFERS_COUNT - 3);
    }
}

int getRandomNumber(int delta, int center) {
    int result = (center - delta) + std::rand() % ((center + delta) - (center - delta) + 1);
    return result;
}

void playSound(int key) {
    for (int i = 0; i < SOUNDS_COUNT; i++) {
        if (sounds[i].getStatus() != sf::Sound::Playing) {
            sounds[i].setBuffer(buffers[selectSound(key)]);
            // set random volume
            int volume = getRandomNumber(config.volumeMaxDelta, config.volume);
            if (volume > 100) volume = 100;
            else if (volume < 0) volume = 0;
            std::cout << "Current volume - " << volume << "\n";
            sounds[i].setVolume(volume);
            float pitch = (float)getRandomNumber(config.pitchMaxDelta*100, 100)/100;
            std::cout << "Current pitch - " << pitch << "\n";
            sounds[i].setPitch(pitch);
            sounds[i].play();
            return;
        }
    }
}

std::chrono::high_resolution_clock::time_point getCurrentTime() {
    return std::chrono::high_resolution_clock::now();
}

std::chrono::duration<double> calculateDeltaTime(std::chrono::high_resolution_clock::time_point start_time, std::chrono::high_resolution_clock::time_point end_time) {
    return std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (wParam == WM_KEYDOWN) {
            KBDLLHOOKSTRUCT* pHookStruct = (KBDLLHOOKSTRUCT*)lParam;
            DWORD keyCode = pHookStruct->vkCode;
            std::cout << "Key pressed: " << keyCode << std::endl;
                        
            std::chrono::high_resolution_clock::time_point currentTime = getCurrentTime();
            std::chrono::duration<double> delta = calculateDeltaTime(prevClickTime, currentTime);

            if (!(delta.count() < 0.04 && (int)keyCode == prevCharacter))
                playSound((int)keyCode);
            prevClickTime = currentTime;
            prevCharacter = (int)keyCode;
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    if (getConfig() == 1) {
        std::cerr << "Reading config file error!";
        getchar();
    }
    if (initSounds() == 1) {
        std::cerr << "Init sounds error!";
        getchar();
    }
    prevCharacter = 0;
    prevClickTime = getCurrentTime();
    

    // Set up keyboard hook
    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (keyboardHook == NULL) {
        std::cerr << "Failed to set keyboard hook" << std::endl;
        getchar();
        return 1;
    }

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up
    UnhookWindowsHookEx(keyboardHook);
    return 0;
}