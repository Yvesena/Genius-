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

using namespace std;
using namespace cv;

bool jaSalvo = false;
int blockSize = 20;
int pontuacao = 0;
Mat cobrinha;
Mat background; // Plano de fundo

// Posição da "cobra"
Point snakePos(100, 100);
Point comida;

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
    comida = Point(rand() % 30 * blockSize, rand() % 20 * blockSize); // Gera comida aleatória
}

// Desenha a comida
void drawFood(Mat& frame) {
    rectangle(frame, comida, Point(comida.x + blockSize, comida.y + blockSize), Scalar(0, 0, 255), FILLED);
}

// Função para desenhar a imagem da "cobra"
void drawSnake(Mat& frame) {
    if (!cobrinha.empty()) {
        Mat mask, snakeResized;
        resize(cobrinha, snakeResized, Size(blockSize * 2, blockSize * 2)); // Ajusta o tamanho da "cobra"
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

// Atualiza a posição da cobra com base na detecção do rosto
void updateGame(Point faceCenter) {
    // Atualiza a posição da cabeça da cobra (o rosto detectado) com uma suavização
    snakePos = Point(faceCenter.x - blockSize, faceCenter.y - blockSize);

    // Verifica se comeu a comida
    if (abs(snakePos.x - comida.x) < blockSize && abs(snakePos.y - comida.y) < blockSize) {
        pontuacao += 10;
        comida = Point(rand() % 30 * blockSize, rand() % 20 * blockSize);
    }
}

int main(int argc, const char** argv) {
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

    // Carrega a imagem de plano de fundo
    background = imread("background.jpg", IMREAD_COLOR);
    if (background.empty()) {
        cout << "ERROR: Could not load background image." << endl;
        return -1;
    }
    resize(background, background, Size(640, 480));

    if (!capture.open(0)) {
        cout << "Capture from camera #0 didn't work" << endl;
        return 1;
    }
    capture.set(CAP_PROP_FRAME_WIDTH, 640);
    capture.set(CAP_PROP_FRAME_HEIGHT, 480);

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
        faceCascade.detectMultiScale(grayFrame, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(30, 30));
        
        if (!faces.empty()) {
            // Usa o centro do primeiro rosto detectado como a posição da cobra
            Point faceCenter(faces[0].x + faces[0].width / 2, faces[0].y + faces[0].height / 2);
            updateGame(faceCenter);
        }

        // Copia a imagem de fundo para exibição
        Mat displayFrame = background.clone();

        // Desenha a "cobra" (imagem da cabeça) e a comida no plano de fundo
        drawSnake(displayFrame);
        drawFood(displayFrame);

        // Exibe o placar
        putText(displayFrame, "Score: " + to_string(pontuacao), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);

        // Exibe o frame com a cobra e comida
        imshow("Snake Game", displayFrame);

        char key = (char)waitKey(30);
        if (key == 27) {  // ESC para sair
            Arquivo(pontuacao);
            break;
        }
    }

    return 0;
}
