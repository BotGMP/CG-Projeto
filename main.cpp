// Incluir cabeçalhos padrão
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>

// Incluir GLEW
#include <GL/glew.h>

// Incluir GLFW
#include <GLFW/glfw3.h>
GLFWwindow *window;

// Incluir GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

// Incluir o carregador de OBJ
#include "common/tiny_obj_loader.h"

// Incluir os shader e texture
#include <common/shader.hpp>
#include "common/texture.hpp"

#define MAX_PROJETEIS 100

//////////////////////////////////////////////////

GLuint hangarTexture;
GLuint hangarUVBuffer;

GLuint falconTexture;
GLuint falconUVBuffer;

GLuint enemyShipTexture;
GLuint enemyShipUVBuffer;

GLuint cuboTexture;
GLuint cuboUVBuffer;

GLuint VertexArrayID;
GLuint programID;
GLuint MatrixID;
glm::mat4 Projection;
glm::mat4 View;
GLuint hangarColorBuffer;

GLuint colorbuffer;
std::vector<float> colors;

// Buffers
GLuint falconVAO, hangarVAO;
GLuint vertexbuffer, normalbuffer;
GLuint hangarVertexBuffer, hangarNormalBuffer;
GLuint enemyVertexBuffer, enemyNormalBuffer, enemyColorBuffer;
GLuint cuboVertexBuffer, cuboNormalBuffer, cuboColorBuffer;

glm::mat4 MVP;

// Dimensões da janela
GLint WindowWidth = 1920;
GLint WindowHeight = 1080;

// Para movimento da nave
float modelX = 00.0f;
float modelZ = 10.0f;
float modelRotationY = 0.0f;
float modelSpeed = 0.2f;
float score = 0.0f;
float lastScore = 0.0f;

// Dados do modelo
std::vector<float> vertices;
std::vector<float> normals;
std::vector<float> hangarVertices;
std::vector<float> hangarNormals;
std::vector<float> enemyVertices;
std::vector<float> enemyNormals;
std::vector<float> cuboVertices;
std::vector<float> cuboNormals;

GLuint vertexBuffer, uvBuffer, normalBuffer;
GLuint texture;

// Nave viva?
bool estouVivo = true;

// Hitboxes
float falconRadius = 0.32f;
float projectileRadius = 0.50f;

struct Enemy
{
    glm::vec3 position;
    float radius;
    bool isAlive;
    float deathTime;
};

std::vector<Enemy> enemies;

/////////////////////////////////////////////////////////////////////////////////

// Geração das cores
void gerarCinzentos(std::vector<float> &out_colors, size_t vertex_count, float baseGrey)
{
    for (size_t i = 0; i < vertex_count; ++i)
    {
        float grey = baseGrey;
        out_colors.push_back(grey);
        out_colors.push_back(grey);
        out_colors.push_back(grey);
    }
}

// Função para carregar o modelo OBJ
bool loadOBJ(const char *path, std::vector<float> &out_vertices, std::vector<float> &out_normals, std::vector<float> &out_uvs)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path);
    if (!ret)
    {
        fprintf(stderr, "O Obj não carregou bem: %s\n", path);
        return false;
    }

    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            out_vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
            out_vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
            out_vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);

            out_normals.push_back(attrib.normals[3 * index.normal_index + 0]);
            out_normals.push_back(attrib.normals[3 * index.normal_index + 1]);
            out_normals.push_back(attrib.normals[3 * index.normal_index + 2]);

            // Add UV coordinates
            out_uvs.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
            out_uvs.push_back(attrib.texcoords[2 * index.texcoord_index + 1]);
        }
    }
    return true;
}

void transferDataToGPUMemory(void)
{
    // VAO
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Carregar shaders program
    programID = LoadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");

    // Carregar a nave
    std::vector<float> falconUVs;
    if (!loadOBJ("falcon.obj", vertices, normals, falconUVs))
    {
        fprintf(stderr, "Erro ao carregar o falcon\n");
        exit(-1);
    }

    // Load falcon texture
    falconTexture = loadDDS("falcon.dds");

    // Create and fill UV buffer
    glGenBuffers(1, &falconUVBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, falconUVBuffer);
    glBufferData(GL_ARRAY_BUFFER, falconUVs.size() * sizeof(float), &falconUVs[0], GL_STATIC_DRAW);

    // Cores para a nave
    gerarCinzentos(colors, vertices.size() / 3, 0.7f);

    // Vertex buffer da nave
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    // Normal buffer da nave
    glGenBuffers(1, &normalbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);

    // Color buffer da nave
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), &colors[0], GL_STATIC_DRAW);

    ///////////////////////////////////////////////////
    // Carregar inimigos
    std::vector<float> enemyShipUVs;
    if (!loadOBJ("enemyShip.obj", enemyVertices, enemyNormals, enemyShipUVs))
    {
        fprintf(stderr, "Erro ao carregar as enemyShips\n");
        exit(-1);
    }

    enemyShipTexture = loadDDS("attack.dds");

    // Create and fill UV buffer
    glGenBuffers(1, &enemyShipUVBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, enemyShipUVBuffer);
    glBufferData(GL_ARRAY_BUFFER, enemyShipUVs.size() * sizeof(float), &enemyShipUVs[0], GL_STATIC_DRAW);

    // Cores para a nave
    std::vector<float> enemyColors;
    gerarCinzentos(enemyColors, enemyVertices.size() / 3, 0.7f);

    // Vertex buffer da nave
    glGenBuffers(1, &enemyVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, enemyVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, enemyVertices.size() * sizeof(float), &enemyVertices[0], GL_STATIC_DRAW);

    // Normal buffer da nave
    glGenBuffers(1, &enemyNormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, enemyNormalBuffer);
    glBufferData(GL_ARRAY_BUFFER, enemyNormals.size() * sizeof(float), &enemyNormals[0], GL_STATIC_DRAW);

    // Color buffer da nave
    glGenBuffers(1, &enemyColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, enemyColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, enemyColors.size() * sizeof(float), &enemyColors[0], GL_STATIC_DRAW);

    ////////////////////////////////////////////////////////////
    // Carregar projeteis
    std::vector<float> cuboUVs;
    if (!loadOBJ("cubo.obj", cuboVertices, cuboNormals, cuboUVs))
    {
        fprintf(stderr, "Erro ao carreagar os projeteis\n");
        exit(-1);
    }

    cuboTexture = loadDDS("projectils.dds");

    // Create and fill UV buffer
    glGenBuffers(1, &cuboUVBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, cuboUVBuffer);
    glBufferData(GL_ARRAY_BUFFER, cuboUVs.size() * sizeof(float), &cuboUVs[0], GL_STATIC_DRAW);

    // Cores para a nave
    std::vector<float> cuboColors;
    gerarCinzentos(cuboColors, cuboVertices.size() / 3, 0.7f);

    // Vertex buffer da nave
    glGenBuffers(1, &cuboVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, cuboVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, cuboVertices.size() * sizeof(float), &cuboVertices[0], GL_STATIC_DRAW);

    // Normal buffer da nave
    glGenBuffers(1, &cuboNormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, cuboNormalBuffer);
    glBufferData(GL_ARRAY_BUFFER, cuboNormals.size() * sizeof(float), &cuboNormals[0], GL_STATIC_DRAW);

    // Color buffer da nave
    glGenBuffers(1, &cuboColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, cuboColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, cuboColors.size() * sizeof(float), &cuboColors[0], GL_STATIC_DRAW);

    /////////////////////////////////////////////////////////
    // Carregar o hangar
    std::vector<float> hangarUVs;
    if (!loadOBJ("hangar.obj", hangarVertices, hangarNormals, hangarUVs))
    {
        fprintf(stderr, "Erro ao carregar o hangar\n");
        exit(-1);
    }

    // Load hangar texture
    hangarTexture = loadDDS("hangar.dds");

    // Create and fill UV buffer
    glGenBuffers(1, &hangarUVBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, hangarUVBuffer);
    glBufferData(GL_ARRAY_BUFFER, hangarUVs.size() * sizeof(float), &hangarUVs[0], GL_STATIC_DRAW);

    // Cores para o hangar
    std::vector<float> hangarColors;
    gerarCinzentos(hangarColors, hangarVertices.size() / 3, 0.5f);

    // Vertex buffer do hangar
    glGenBuffers(1, &hangarVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, hangarVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, hangarVertices.size() * sizeof(float), &hangarVertices[0], GL_STATIC_DRAW);

    // Normal buffer do hangar
    glGenBuffers(1, &hangarNormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, hangarNormalBuffer);
    glBufferData(GL_ARRAY_BUFFER, hangarNormals.size() * sizeof(float), &hangarNormals[0], GL_STATIC_DRAW);

    // Color buffer do hangar
    glGenBuffers(1, &hangarColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, hangarColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, hangarColors.size() * sizeof(float), &hangarColors[0], GL_STATIC_DRAW);

    // Carregar os dados
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glBindVertexArray(0);
}

/////////////////////////////////////////////////////////

// Projeteis
struct Projectile
{
    glm::vec3 position;
    glm::vec3 direction;
    float speed;
};
std::vector<Projectile> projectiles;
// Criar um vetor separado para os projéteis do Falcon
std::vector<Projectile> falconProjectiles;
float projectileSpeed = 0.06f; // Speed of the projectiles

void shootProjectile(glm::vec3 startPosition)
{
    Projectile p;
    p.position = startPosition;
    p.direction = glm::vec3(0.0f, 0.0f, 1.0f); // Shoots straight down in the z-direction
    p.speed = projectileSpeed;
    projectiles.push_back(p);
}

// Colisão
bool checkCollision(const glm::vec3 &object1Pos, float object1Radius,
                    const glm::vec3 &object2Pos, float object2Radius)
{
    float distance = glm::distance(object1Pos, object2Pos);
    return distance <= (object1Radius + object2Radius);
}

///////////////////////////////////////////

float lastFalconShotTime = 0.0f;
float falconShotCooldown = 1.0f; // Tempo mínimo entre disparos (em segundos)

void shootFalconProjectile(const glm::vec3 &position)
{
    Projectile newProjectile;
    newProjectile.position = position;
    newProjectile.direction = glm::vec3(0.0f, 0.0f, 1.0f); // Direção positiva em Z
    newProjectile.speed = -0.10f;                          // Velocidade do projétil
    falconProjectiles.push_back(newProjectile);
}

// Funcao para o movimento da nave
void controloNave(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if ((action == GLFW_PRESS || action == GLFW_REPEAT) && estouVivo == true)
    {
        if (key == GLFW_KEY_D && modelX < 5)
        {
            modelX += modelSpeed;
        }
        if (key == GLFW_KEY_A && modelX > -5)
        {
            modelX -= modelSpeed;
        }
        if (key == GLFW_KEY_SPACE)
        {
            float currentTime = glfwGetTime();
            if (currentTime - lastFalconShotTime >= falconShotCooldown)
            {
                shootFalconProjectile(glm::vec3(modelX, 0.0f, modelZ));
                lastFalconShotTime = currentTime;
            }
        }
    }
    if ((action == GLFW_PRESS || action == GLFW_REPEAT) && estouVivo == false)
    {
        if (key == GLFW_KEY_R)
        {
            modelX = 0.0f;
            modelZ = 10.0f;
            estouVivo = true;
            for (auto &enemy : enemies)
            {
                if (enemy.isAlive == false)
                {
                    enemy.isAlive = true;
                }
            }
            projectiles.clear();
            falconProjectiles.clear();
            lastScore = score;
        }
    }
}

// Funcao para desenhar o modelo recebido
void drawModel(GLuint vertexbuffer, GLuint normalbuffer, GLuint colorbuffer, GLuint uvbuffer, GLuint texture,
               std::vector<float>::size_type vertex_count, const glm::mat4 &modelMatrix)
{
    glm::mat4 ModelViewMatrix = View * modelMatrix;
    glm::mat3 NormalMatrix = glm::mat3(glm::transpose(glm::inverse(ModelViewMatrix)));

    MVP = Projection * ModelViewMatrix;

    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, value_ptr(MVP));
    GLuint ModelViewMatrixID = glGetUniformLocation(programID, "ModelViewMatrix");
    GLuint NormalMatrixID = glGetUniformLocation(programID, "NormalMatrix");
    glUniformMatrix4fv(ModelViewMatrixID, 1, GL_FALSE, value_ptr(ModelViewMatrix));
    glUniformMatrix3fv(NormalMatrixID, 1, GL_FALSE, value_ptr(NormalMatrix));

    if (texture != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    if (uvbuffer != 0)
    {
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    }
    else
    {
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    }

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glDrawArrays(GL_TRIANGLES, 0, vertex_count);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

int main(void)
{
    // Initialize GLFW and create window
    if (!glfwInit())
        return -1;
    GLFWwindow *window = glfwCreateWindow(WindowWidth, WindowHeight, "3D Models", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewInit();
    glfwSetKeyCallback(window, controloNave);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Transfer data to GPU
    transferDataToGPUMemory();

    // Obtain the MVP uniform ID
    MatrixID = glGetUniformLocation(programID, "MVP");

    // Escala dos hangar
    float hangarScale = 0.08f;
    float hangarRotationAngle = 270.0f;
    float falconScale = 0.40f;

    // Parametros naves
    int gridRows = 5;
    int gridCols = 5;
    float gridSpacing = 2.0f;
    float enemyScale = 0.35f;
    float enemyRotationAngle = 90.0f;
    float oscillationAmplitude = 2.0f;
    float oscillationSpeed = 0.5f;
    const float respawnTime = 10.0f;

    // Inicializar inimigos na grade
    for (int row = 0; row < gridRows; ++row)
    {
        for (int col = 0; col < gridCols; ++col)
        {
            Enemy enemy;
            enemy.position = glm::vec3(col * gridSpacing - 5, 0.0f, row * gridSpacing - 5);
            enemy.radius = 0.35f;
            enemy.isAlive = true;
            enemies.push_back(enemy);
        }
    }

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(programID);
        float time = glfwGetTime();
        score = time - lastScore;

        // Parametros da iluminação
        glm::vec3 lightPosition(4.0f, 4.0f, 4.0f);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        glm::vec3 ambientColor(0.2f, 0.2f, 0.2f);
        float shininess = 32.0f;
        float specularStrength = 0.5f;

        GLuint lightPosID = glGetUniformLocation(programID, "lightPosition_cameraSpace");
        GLuint lightColorID = glGetUniformLocation(programID, "lightColor");
        GLuint ambientColorID = glGetUniformLocation(programID, "ambientColor");
        GLuint shininessID = glGetUniformLocation(programID, "shininess");
        GLuint strengthID = glGetUniformLocation(programID, "strength");

        // Passar dados da iluminacao para o shader
        glm::vec3 lightPos_cameraSpace = glm::vec3(View * glm::vec4(lightPosition, 1.0));
        glUniform3fv(lightPosID, 1, glm::value_ptr(lightPos_cameraSpace));
        glUniform3fv(lightColorID, 1, glm::value_ptr(lightColor));
        glUniform3fv(ambientColorID, 1, glm::value_ptr(ambientColor));
        glUniform1f(shininessID, shininess);
        glUniform1f(strengthID, specularStrength);

        // Camara
        Projection = glm::perspective(glm::radians(90.0f), (float)WindowWidth / (float)WindowHeight, 0.1f, 100.0f);
        View = glm::lookAt(glm::vec3(3, 7, 16), glm::vec3(0, 0, 0), glm::vec3(0, 0.5, 0));

        // First hangar
        drawModel(hangarVertexBuffer, hangarNormalBuffer, hangarColorBuffer, hangarUVBuffer, hangarTexture,
                  hangarVertices.size() / 3,
                  glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 40.0f)) *
                      glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(hangarScale, hangarScale, hangarScale)));

        // Second hangar
        drawModel(hangarVertexBuffer, hangarNormalBuffer, hangarColorBuffer, hangarUVBuffer, hangarTexture,
                  hangarVertices.size() / 3,
                  glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -18.0f)) *
                      glm::rotate(glm::mat4(1.0f), glm::radians(hangarRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(hangarScale, hangarScale, hangarScale)));

        // Desenhar nave
        if (estouVivo == true)
        {
            glm::mat4 falconModel = glm::translate(glm::mat4(1.0f), glm::vec3(modelX, 0.0f, modelZ));
            falconModel = glm::rotate(falconModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            falconModel = glm::scale(falconModel, glm::vec3(falconScale, falconScale, falconScale));
            drawModel(vertexbuffer, normalbuffer, colorbuffer, falconUVBuffer, falconTexture, vertices.size() / 3, falconModel);
        }

        // Desenhar e reviver naves inimigas
        for (size_t i = 0; i < enemies.size(); ++i)
        {
            if (!enemies[i].isAlive)
            {
                float timeSinceDeath = time - enemies[i].deathTime;
                if (timeSinceDeath >= respawnTime)
                {
                    enemies[i].isAlive = true;
                }
                else
                {
                    continue;
                }
            }
			
			if(enemies[i].isAlive){
            // Calcular a oscilação para o movimento dinâmico
            float oscillation = oscillationAmplitude * sin(time * oscillationSpeed);
            glm::vec3 dynamicPosition = enemies[i].position;
            dynamicPosition.x += oscillation; // Oscilação horizontal

            // Aplicar transformações no modelo
            glm::mat4 enemyModel = glm::translate(glm::mat4(1.0f), dynamicPosition) *
                                   glm::rotate(glm::mat4(1.0f), glm::radians(enemyRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)) *
                                   glm::scale(glm::mat4(1.0f), glm::vec3(enemyScale, enemyScale, enemyScale));

            drawModel(enemyVertexBuffer, enemyNormalBuffer, enemyColorBuffer, enemyShipUVBuffer, enemyShipTexture, enemyVertices.size() / 3, enemyModel);
		}
            
        }

        /////////////////////////////////////////////////////
        // Projeteis
        static float lastShootTime = 0.0f;
        float shootInterval = 3.0f;

        if (time - lastShootTime >= shootInterval)
        {
            lastShootTime = time;

            // Iterar por todas as colunas na grade
            for (int col = 0; col < gridCols; ++col)
            {
                // Procurar a primeira nave viva de baixo para cima em cada coluna
                for (int row = gridRows - 1; row >= 0; --row)
                {
                    // Calcular o índice correspondente no vetor unidimensional
                    int index = row * gridCols + col;

                    if (enemies[index].isAlive)
                    {
                        // Obter a posição dinâmica do inimigo
                        float oscillation = oscillationAmplitude * sin(time * oscillationSpeed);
                        glm::vec3 shipPosition = enemies[index].position;

                        // Adicionar oscilação à posição horizontal
                        shipPosition.x += oscillation;

                        // Disparar o projétil do inimigo encontrado
                        shootProjectile(shipPosition);
                        break; // Sair do loop interno, pois encontramos a nave que deve disparar
                    }
                }
            }
        }

        // Update posicao tiros
        for (auto &projectile : projectiles)
        {
            projectile.position += projectile.direction * projectile.speed;
        }

        // Update posicao tiros falcon
        for (auto &projectile : falconProjectiles)
        {
            projectile.position += projectile.direction * projectile.speed;
        }

        // Desenhar projectil
        for (const auto &projectile : projectiles)
        {
            glm::mat4 projectileModel = glm::translate(glm::mat4(1.0f), projectile.position) *
                                        glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));                                                // Small scale for projectile
            drawModel(cuboVertexBuffer, cuboNormalBuffer, cuboColorBuffer, cuboUVBuffer, cuboTexture, cuboVertices.size() / 3, projectileModel); // Use cube model
        }

        // Desenhar projetils falcon
        for (const auto &projectile : falconProjectiles)
        {
            glm::mat4 projectileModelF = glm::translate(glm::mat4(1.0f), projectile.position) *
                                         glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));                                                // Small scale for projectile
            drawModel(cuboVertexBuffer, cuboNormalBuffer, cuboColorBuffer, cuboUVBuffer, cuboTexture, cuboVertices.size() / 3, projectileModelF); // Use cube model
        }

        // Apagar projeteis fora
        projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                         [](const Projectile &p)
                                         { return p.position.z < -50.0f; }), // Assume -50.0f is out of bounds
                          projectiles.end());

        falconProjectiles.erase(
            std::remove_if(falconProjectiles.begin(), falconProjectiles.end(), [](const Projectile &proj)
                           { return proj.position.z > 50.0f || proj.position.z < -50.0f; }),
            falconProjectiles.end());

        // Posição atual da nave Falcon
        glm::vec3 falconPosition = glm::vec3(modelX, 0.0f, modelZ);

        // deslocamento ao centro da hitbox
        glm::vec3 collisionOffset = glm::vec3(-0.2f, 0.0f, 1.6f);
        glm::vec3 collisionCenter = falconPosition + collisionOffset;

        // Verificar colisões
        if (estouVivo == true)
        {
            for (auto it = projectiles.begin(); it != projectiles.end();)
            {
                if (checkCollision(collisionCenter, falconRadius, it->position, projectileRadius))
                {
                    // Remove o projétil da lista após a colisão
                    it = projectiles.erase(it);
                    estouVivo = false;
                    std::cout << "Score: " << score << std::endl;
                }
                else
                {
                    ++it; // Próximo projétil
                }
            }
        }

        for (auto &enemy : enemies)
        {
            if (enemy.isAlive)
            {
                // Calcular a posição dinâmica com oscilação
                float oscillation = oscillationAmplitude * sin(time * oscillationSpeed);
                glm::vec3 dynamicPosition = enemy.position;
                dynamicPosition.x += oscillation; // Aplicar deslocamento horizontal

                // Atualizar a hitbox com a posição dinâmica
                glm::vec3 adjustedEnemyPosition = dynamicPosition + glm::vec3(0.5f, 0.0f, 0.0f);

                // Verificar colisão com projéteis do Falcon
                for (auto &proj : falconProjectiles)
                {
                    if (checkCollision(proj.position, projectileRadius, adjustedEnemyPosition, enemy.radius))
                    {
                        enemy.isAlive = false;     // Marca como destruído
                        proj.position.z = 1000.0f; // Remove o projétil
                    }
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &normalbuffer);
    glDeleteBuffers(1, &hangarVertexBuffer);
    glDeleteBuffers(1, &hangarNormalBuffer);
    glDeleteBuffers(1, &colorbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
