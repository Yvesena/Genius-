#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <chrono>
#include <thread>
#include <vector>
#include <fstream>
#include <iostream>
#include <deque>
#include <string>
#include <locale>
#include <bits/stdc++.h>

using namespace std;
using namespace cv;

bool jaSalvo = false;
int blockSize = 20;
int pontuacao = 0;
Mat cobrinha;     // imagem da cobrinha
Mat background;   // Plano de fundo
Mat comidaImage;  //Imagem de comida
Mat comidaImage2;  // Imagem da comida2

// Posição da "cobrinha" e da comida
Point snakePos(100, 100);
Point comida; //
Point comida2; // posição da comida
Point chaserPos(20, 20); // Posição inicial do perseguidor
int chaserSpeed = 2; // Velocidade do perseguidor

// Função para salvar a pontuação
void Arquivo(int pontuacao) {
    fstream file;
    string linha;
    file.open("tabela_de_pontuacoes.txt", ios::in);
    if (file.is_open()) {
        while (getline(file, linha)) {
            if (to_string(pontuacao) == linha) {
                jaSalvo = true;
                break;
            }
        }
        file.close();
    }
    if (!jaSalvo) {
        file.open("tabela_de_pontuacoes.txt", ios::app);
        if (!file) {
            cout << "Erro" << endl;
        } else {
            file << pontuacao << endl;
            cout << "Pontuação salva" << endl;
            file.close();
        }
    }
}

// Função para reiniciar o jogo
void ReiniciarGame() {
    pontuacao = 0;
    jaSalvo = false;
    comida = Point(rand() % 30 * blockSize, rand() % 20 * blockSize);  // Gera a posição da primeira comida
    comida2 = Point(rand() % 30 * blockSize, rand() % 20 * blockSize); // Gera a posição da segunda comida
    chaserPos = Point(20, 20); // Reinicia a posição do perseguidor
}

// Desenha a comida com uma imagem PNG
void drawFood(Mat& frame) {
    // Desenha a primeira comida
    if (!comidaImage.empty()) {
        Mat mask, comidaResized;
        int foodSize = blockSize * 2;
        resize(comidaImage, comidaResized, Size(foodSize, foodSize));
        vector<Mat> layers;
        split(comidaResized, layers);
        if (layers.size() == 4) {
            Mat rgb[3] = {layers[0], layers[1], layers[2]};
            mask = layers[3];
            merge(rgb, 3, comidaResized);
            comidaResized.copyTo(frame(Rect(comida.x, comida.y, comidaResized.cols, comidaResized.rows)), mask);
        } else {
            comidaResized.copyTo(frame(Rect(comida.x, comida.y, comidaResized.cols, comidaResized.rows)));
        }
    }

    // Desenha a segunda comida
    if (!comidaImage2.empty()) {
        Mat mask, comidaResized2;
        int foodSize2 = blockSize * 2;
        resize(comidaImage2, comidaResized2, Size(foodSize2, foodSize2));
        vector<Mat> layers2;
        split(comidaResized2, layers2);
        if (layers2.size() == 4) {
            Mat rgb2[3] = {layers2[0], layers2[1], layers2[2]};
            mask = layers2[3];
            merge(rgb2, 3, comidaResized2);
            comidaResized2.copyTo(frame(Rect(comida2.x, comida2.y, comidaResized2.cols, comidaResized2.rows)), mask);
        } else {
            comidaResized2.copyTo(frame(Rect(comida2.x, comida2.y, comidaResized2.cols, comidaResized2.rows)));
        }
    }
}

// Função para desenhar a imagem da "cobra"
void drawSnake(Mat& frame) {
    if (!cobrinha.empty()) {
        Mat mask, snakeResized;
        int snakeSize = blockSize * 2; // Tamanho constante da cobra
        resize(cobrinha, snakeResized, Size(snakeSize, snakeSize)); // Ajusta o tamanho da cobra
        vector<Mat> layers;
        split(snakeResized, layers);  // Separa os canais de cor
        if (layers.size() == 4) {     // Imagem com transparência
            Mat rgb[3] = {layers[0], layers[1], layers[2]};
            mask = layers[3];         // Canal alfa (transparência)
            merge(rgb, 3, snakeResized);  // Junta os canais RGB
            snakeResized.copyTo(frame(Rect(snakePos.x, snakePos.y, snakeResized.cols, snakeResized.rows)), mask);
        } else {
            snakeResized.copyTo(frame(Rect(snakePos.x, snakePos.y, snakeResized.cols, snakeResized.rows)));
        }
    }
}

// Função para desenhar o perseguidor
void drawChaser(Mat& frame) {
    Scalar chaserColor(0, 0, 255); // Cor vermelha para o perseguidor
    Point chaserTopLeft(chaserPos.x, chaserPos.y);
    Point chaserBottomRight(chaserPos.x + blockSize, chaserPos.y + blockSize);
    rectangle(frame, chaserTopLeft, chaserBottomRight, chaserColor, -1); // Desenha o perseguidor
}

// Atualiza a posição da cobra com base na detecção do rosto
void updateGame(Point faceCenter) {
    // Atualiza a posição da cabeça da cobra
    snakePos = Point(faceCenter.x - blockSize, faceCenter.y - blockSize);

    // Verifica se comeu a primeira comida
    if (abs(snakePos.x - comida.x) < blockSize && abs(snakePos.y - comida.y) < blockSize) {
        pontuacao += 10;
        comida = Point(rand() % 30 * blockSize, rand() % 20 * blockSize); // Gera nova posição para a primeira comida
    }

    // Verifica se comeu a segunda comida
    if (abs(snakePos.x - comida2.x) < blockSize && abs(snakePos.y - comida2.y) < blockSize) {
        pontuacao += 10;
        comida2 = Point(rand() % 30 * blockSize, rand() % 20 * blockSize); // Gera nova posição para a segunda comida
    }
}

// Atualiza a posição do perseguidor
void updateChaser() {
    // Move o perseguidor em direção à cobra
    if (chaserPos.x < snakePos.x) {
        chaserPos.x += chaserSpeed; // Move para a direita
    } else if (chaserPos.x > snakePos.x) {
        chaserPos.x -= chaserSpeed; // Move para a esquerda
    }

    if (chaserPos.y < snakePos.y) {
        chaserPos.y += chaserSpeed; // Move para baixo
    } else if (chaserPos.y > snakePos.y) {
        chaserPos.y -= chaserSpeed; // Move para cima
    }

    // Verifica se o perseguidor alcançou a cobrinha
    if (abs(chaserPos.x - snakePos.x) < blockSize && abs(chaserPos.y - snakePos.y) < blockSize) {
        cout << "Game Over! O perseguidor alcançou a cobrinha." << endl;
        Arquivo(pontuacao); // Salva a pontuação
        exit(0); // Encerra o jogo
    }
}

int main(int argc, const char** argv) {
    srand(time(0));
    VideoCapture capture;
    Mat frame;
    CascadeClassifier faceCascade;
    string faceCascadeName = "haarcascade_frontalface_default.xml";

    // Carrega o Haar Cascade para detecção de rostos
    if (!faceCascade.load(faceCascadeName)) {
        cout << "ERROR: Could not load classifier cascade: " << faceCascadeName << endl;
        return -1;
    }

    // Carrega a imagem que será usada como "cobra"
    cobrinha = imread("cobrinha.png", IMREAD_UNCHANGED);
    if (cobrinha.empty()) {
        cout << "ERROR: Could not load snake image." << endl;
        return -1;
    }

    // Carrega a imagem de comida
    comidaImage = imread("comida.png", IMREAD_UNCHANGED); 
    if (comidaImage.empty()) {
        cout << "ERROR: Could not load food image." << endl;
        return -1;
    }

    comidaImage2 = imread("comida2.png", IMREAD_UNCHANGED);  
    if (comidaImage2.empty()) {
        cout << "ERROR: Could not load second food image." << endl;
        return -1;
    }

    // Carrega a imagem de plano de fundo
    background = imread("background.jpg", IMREAD_COLOR);
    if (background.empty()) {
        cout << "ERROR: Could not load background image." << endl;
        return -1;
    }
    int windowWidth = 640;  // Altere conforme necessário
    int windowHeight = 480; // Altere conforme necessário
    resize(background, background, Size(windowWidth, windowHeight));  // Redimensiona o plano de fundo

    if (!capture.open(0)) {
        cout << "Capture from camera #0 didn't work" << endl;
        return 1;
    }
    capture.set(CAP_PROP_FRAME_WIDTH, windowWidth);  // Define a largura da captura de vídeo
    capture.set(CAP_PROP_FRAME_HEIGHT, windowHeight); // Define a altura da captura de vídeo

    ReiniciarGame();

    while (true) {
        capture >> frame;
        if (frame.empty())
            break;

        flip(frame, frame, 1);

        Mat grayFrame;
        cvtColor(frame, grayFrame, COLOR_BGR2GRAY);
        vector<Rect> faces;

        // Detecta rostos na imagem
        faceCascade.detectMultiScale(grayFrame, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(100, 100));

        if (!faces.empty()) {
            // Usa o centro do primeiro rosto detectado como a posição da cobra
            Point faceCenter(faces[0].x + faces[0].width / 4, faces[0].y + faces[0].height / 4);
            updateGame(faceCenter);
        }

        // Atualiza a posição do perseguidor
        updateChaser();

        // Copia a imagem de fundo para exibição
        Mat displayFrame = background.clone();

        // Desenha a "cobra" (imagem da cabeça), a comida e o perseguidor no plano de fundo
        drawSnake(displayFrame);
        drawFood(displayFrame);
        drawChaser(displayFrame);

        // Exibe o placar
        putText(displayFrame, "Placar: " + to_string(pontuacao), Point(430,470 ), FONT_HERSHEY_SIMPLEX, 1, Scalar(128, 0, 128), 2);

        // Desenha retângulos ao redor dos rostos detectados
        for (size_t i = 0; i < faces.size(); i++) {
            Rect r = faces[i];
            rectangle(frame, Point(cvRound(r.x), cvRound(r.y)),
                      Point(cvRound((r.x + r.width - 1)), cvRound((r.y + r.height - 1))),
                      Scalar(255, 0, 0), 3); // Desenha um retângulo azul
        }

        // Exibe o frame com a cobra, comida e perseguidor
        imshow("Snake Game", displayFrame);

        char key = (char)waitKey(30);
        if (key == 27) {  // ESC para sair
            Arquivo(pontuacao);
            break;
        }
    }

    return 0;
}
