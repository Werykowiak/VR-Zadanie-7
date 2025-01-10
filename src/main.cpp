#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "linmath.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

using namespace std;

#define M_PI 3.14159265358979323846

float lastX = 320.0f, lastY = 240.0f;  // Początkowa pozycja myszy (środek okna)
float pitch = 0.0f, yaw = -90.0f;  // Kąty obrotu kamery
bool firstMouse = true;  // Flaga sprawdzająca, czy to pierwsze wywołanie funkcji (aby zainicjować wartości)

bool isWKeyPressed = false;
bool isSKeyPressed = false;
bool isDKeyPressed = false;
bool isAKeyPressed = false;
bool isSpaceKeyPressed = false;
bool isCtrKeyPressed = false;
bool isLKeyPressed = false;

float fov = 45.0f;
bool isPlusKeyPressed = false;
bool isMinusKeyPressed = false;

struct Vertex
{
    float x,y,z;
}; 
struct TexCoord
{
    float x,y;
}; 
struct Normal
{
    float x,y,z;
}; 
struct CompleteVertex {
    Vertex position;
    Normal normal;
    TexCoord texCoord;
};
struct Object{
    vector<CompleteVertex> vertices;
    vector<unsigned int> indices; 
    map<string, unsigned int> dict;
};
struct Material {
    float Ns;          // Specular exponent
    float Ka[3];       // Ambient color
    float Ks[3];       // Specular color
    float Ke[3];       // Emission color
    float Ni;          // Optical density
    float d;           // Dissolve (transparency)
    int illum;         // Illumination model
    string map_Kd; // Diffuse texture map
};


static const char* vertex_shader_text =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec2 aTexCoord;\n"
"out vec3 vNormal;\n"
"out vec3 vLight;\n"
"out vec3 vEye;\n"
"out vec2 vTexCoord;\n"
"uniform mat4 MVP;\n"
"uniform mat4 MV;\n"
"uniform vec3 lightPos;\n"
"void main()\n"
"{\n"
    "gl_Position = MVP * vec4(aPos, 1.0);\n"
    "mat4 normalMtx = transpose(inverse(MV));\n"
    "vNormal = (normalMtx * vec4(aNormal, 0.0)).xyz;\n"
    "vec4 vPos = MV * vec4(aPos, 1.0);\n"
    "vec4 vLightPos = MV * vec4(lightPos, 1.0);\n"
    "vLight = vLightPos.xyz - vPos.xyz;\n"
    "vEye = -vPos.xyz;\n"
    "vTexCoord = aTexCoord;\n"
"}\n";

static const char* fragment_shader_text =
"#version 330 core\n"
"in vec3 vNormal;\n"
"in vec3 vLight;\n"
"in vec3 vEye;\n"
"in vec2 vTexCoord;\n"
"uniform vec3 Ka;\n"
"uniform vec3 Ks;\n"
"uniform vec3 Ke;\n"
"uniform float Ns;\n"
"uniform sampler2D texture1;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
    "vec3 vNormal_norm = normalize(vNormal);\n"
    "vec3 vLight_norm = normalize(vLight);\n"
    "vec3 vEye_norm = normalize(vEye);\n"
    "vec3 ambient = Ka * vec3(texture(texture1, vTexCoord));\n"
    "float diff = max(dot(vNormal_norm, vLight_norm), 0.0);\n"
    "vec3 diffuse = diff * vec3(texture(texture1, vTexCoord));\n"
    "vec3 vReflect_norm = reflect(-vLight_norm, vNormal_norm);\n"
    "float spec = pow(max(dot(vReflect_norm, vEye_norm), 0.0), Ns);\n"
    "vec3 specular = Ks * spec;\n"
    "vec3 color = ambient + diffuse + specular + Ke;\n"
    "FragColor = vec4(color, 1.0);\n"
"}\n";
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (key == GLFW_KEY_W && action == GLFW_PRESS)
        isWKeyPressed = true;
    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
        isWKeyPressed = false;

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
        isSKeyPressed = true;
    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
        isSKeyPressed = false;

    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        isAKeyPressed = true;
    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
        isAKeyPressed = false;
    
    if (key == GLFW_KEY_D && action == GLFW_PRESS)
        isDKeyPressed = true;
    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
        isDKeyPressed = false;
    
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        isSpaceKeyPressed = true;
    if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
        isSpaceKeyPressed = false;
    
    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS)
        isCtrKeyPressed = true;
    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE)
        isCtrKeyPressed = false;
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
        isLKeyPressed = true;
    if (key == GLFW_KEY_L && action == GLFW_RELEASE)
        isLKeyPressed = false;
    
    if (key == GLFW_KEY_UP && action == GLFW_PRESS && fov<120.0f){
        
        fov += 5.0f;
        printf("zwiekszono FOV: %f\n",fov);
    }
        
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS && fov>20.0f){
        
        fov -= 5.0f;
        printf("mniejszono FOV: %f\n",fov);
    }  
}
float degrees_to_radians(float degrees)
{
    return degrees * (M_PI / 180.0f);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;  // Odwracamy, ponieważ w OpenGL Y rośnie w dół
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;  // Wrażliwość myszy
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
}

void mat4x4_interpolate(mat4x4 result, const mat4x4& a, const mat4x4& b, float t) {
    for (int i = 0; i < 16; ++i) {
        result[0][i] = a[0][i] * (1.0f - t) + b[0][i] * t;
    }
}

GLint mvp_location, mv_location, lightPos_location;

class Node{
    public:
        unsigned int VAO,VBO, EBO;
        mat4x4 currentTransform;
        vector<CompleteVertex> vertices;
        vector<unsigned int> indices; 
        vector<Node> children;
        struct Keyframe {
            mat4x4 transform;
            float time; // Time at which this keyframe occurs
        };
        vector<Keyframe> keyframes;
        Node(){mat4x4_identity(currentTransform);}
        Node(vector<CompleteVertex> vertices, vector<unsigned int> indices){
            this->vertices = vertices; 
            this->indices = indices;   
            glGenVertexArrays(1, &VAO);
            glBindVertexArray(VAO);
            printf("VAO: %d \n",VAO);
            // Tworzenie VBO (dla danych wierzchołków)
            glGenBuffers(1, &VBO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(CompleteVertex), vertices.data(), GL_STATIC_DRAW);

            // Tworzenie EBO (dla indeksów)
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CompleteVertex), (void*)0);
            glEnableVertexAttribArray(0);

            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CompleteVertex), (void*)(sizeof(float) * 3));
            glEnableVertexAttribArray(1);

            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CompleteVertex), (void*)(sizeof(float) * 6));
            glEnableVertexAttribArray(2);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            printf("node Created\n");
            mat4x4_identity(currentTransform);
        }
        void AddKeyframe(const mat4x4& transform, float time) {
            Keyframe keyframe;
            mat4x4_dup(keyframe.transform, transform);
            keyframe.time = time;
            keyframes.push_back(keyframe);
        }
        void Draw(mat4x4 p,mat4x4 lookat, mat4x4 parentTransform){
            mat4x4 mv, mvp, globalTransform;

            // Przemnóż transformację rodzica przez bieżącą transformację
            mat4x4_mul(globalTransform, parentTransform, currentTransform);

            // Oblicz MV i MVP
            mat4x4_mul(mv, lookat, globalTransform);  // MV = LookAt * GlobalTransform
            mat4x4_mul(mvp, p, mv);                            // MVP = Projection * MV

            // Ustaw uniformy shaderów
            glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)mvp);
            glUniformMatrix4fv(mv_location, 1, GL_FALSE, (const GLfloat*)mv);

            // Rysuj bieżący obiekt
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // Rysuj dzieci, przekazując globalną transformację
            for (size_t i = 0; i < children.size(); ++i) {
                children[i].Draw(p,lookat, globalTransform);
            }
        }
        void InterpolateKeyframes(float currentTime) {
            if (keyframes.empty()){
                for (auto& child : children) {
                    child.InterpolateKeyframes(currentTime);
                }
                return;
            } 

            // Znajdź dwie klatki kluczowe do interpolacji
            Keyframe* kf1 = nullptr;
            Keyframe* kf2 = nullptr;

            for (size_t i = 0; i < keyframes.size(); ++i) {
                if (keyframes[i].time >= currentTime) {
                    kf2 = &keyframes[i];
                    if (i > 0) kf1 = &keyframes[i - 1];
                    break;
                }
            }

            if (!kf1 || !kf2) {
                // Użyj pierwszej lub ostatniej klatki kluczowej, jeśli czas jest poza zakresem
                mat4x4_dup(currentTransform, kf1 ? kf1->transform : keyframes.back().transform);
            } else {
                // Oblicz współczynnik interpolacji `t`
                float t = (currentTime - kf1->time) / (kf2->time - kf1->time);

                // Interpoluj macierze transformacji
                mat4x4_interpolate(currentTransform, kf1->transform, kf2->transform, t);
            }

            // Rekurencyjne propagowanie do dzieci
            for (auto& child : children) {
                child.InterpolateKeyframes(currentTime);
            }
        }
        ~Node() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }
};

int main(void)
{
    
    //map<string, unsigned int> dict;
    map<string, Object> objects;


    vector<Vertex> positions;
    vector<TexCoord> texCoords;
    vector<Normal> normals;


    string mtlFileName;

    ifstream obj("object/Tiger.obj");

    if (!obj.is_open()) {
        printf("Error while loading obj file");
        exit(EXIT_FAILURE);
    }
    string line;
    string currentObjectName = "";
    while (getline (obj, line)) {
        string prefix;
        vector<std::string> input;
        istringstream stream(line);
        stream >> prefix;
        if(prefix == "o"){
            stream >> currentObjectName;
            if(objects.find(currentObjectName) == objects.end())\
                objects[currentObjectName] = Object();
        }
        if(prefix == "v"){
            Vertex vertex;
            stream >> vertex.x >> vertex.y >> vertex.z;
            positions.push_back(vertex);
        }else if(prefix == "vn"){
            Normal normal;
            stream >> normal.x >> normal.y>> normal.z;
            normals.push_back(normal);
        }else if(prefix == "vt"){
            TexCoord texCord;
            stream >> texCord.x >> texCord.y;
            texCoords.push_back(texCord);
        }else if(prefix == "f"){
            string vertexDef;
            for(int i=0;i<3;i++){
                stream >> vertexDef;
                if(objects[currentObjectName].dict.find(vertexDef) == objects[currentObjectName].dict.end()){
                    istringstream vs(vertexDef);
                    string vIdx, vtIdx, vnIdx;
                    getline(vs, vIdx, '/');
                    getline(vs, vtIdx, '/');
                    getline(vs, vnIdx, '/');
                    int v = stoi(vIdx) - 1;
                    int vt = stoi(vtIdx) - 1;
                    int vn = stoi(vnIdx) - 1;
                    CompleteVertex completeVertex;
                    completeVertex.position = positions[v];
                    completeVertex.texCoord = texCoords[vt];
                    completeVertex.normal = normals[vn];
                    objects[currentObjectName].vertices.push_back(completeVertex);
                    objects[currentObjectName].dict[vertexDef] = objects[currentObjectName].vertices.size() - 1;
                }
                objects[currentObjectName].indices.push_back(objects[currentObjectName].dict[vertexDef]);

            }
        }else if(prefix=="mtllib"){
            stream >> mtlFileName;
            cout << mtlFileName << endl;
        }
    }
    obj.close();
    cout << "Mtl File name: " << mtlFileName << endl; 

    
    
    

    Material material;

    ifstream mtl("object/"+ mtlFileName);
    if (!mtl.is_open()) {
        cerr << "Failed to open MTL file: " << mtlFileName << endl;
    }

    while (getline(mtl, line)) {
        istringstream iss(line);
        string prefix;
        iss >> prefix;

        if (prefix == "Ns") {
            iss >> material.Ns;
        } else if (prefix == "Ka") {
            iss >> material.Ka[0] >> material.Ka[1] >> material.Ka[2];
        } else if (prefix == "Ks") {
            iss >> material.Ks[0] >> material.Ks[1] >> material.Ks[2];
        } else if (prefix == "Ke") {
            iss >> material.Ke[0] >> material.Ke[1] >> material.Ke[2];
        } else if (prefix == "Ni") {
            iss >> material.Ni;
        } else if (prefix == "d") {
            iss >> material.d;
        } else if (prefix == "illum") {
            iss >> material.illum;
        } else if (prefix == "map_Kd") {
            iss >> material.map_Kd;
        }
    }
    mtl.close();

    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint apos_location, aNormals_location,texCoord_location;

    srand(time(NULL));

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);


    Node* kadlub = nullptr;
    Node* wieza = nullptr;
    Node* lacznik = nullptr;
    Node* lufa = nullptr;
    
    for (const auto& pair : objects) {
        const string& objectName = pair.first;
        const Object& object = pair.second;
        cout << "Object: " << objectName << endl;
        cout << "Vertices: " << object.vertices.size() << endl;
        cout << "Indices: " << object.indices.size() << endl;
        if (objectName == "lufa") {
            lufa = new Node(object.vertices, object.indices);
        } else if (objectName == "Wieza") {
            wieza = new Node(object.vertices, object.indices);
        } else if (objectName == "kadlub") {
            kadlub = new Node(object.vertices, object.indices);
        } else if (objectName == "lacznik") {
            lacznik = new Node(object.vertices, object.indices);
        }
    }
    lacznik->children.push_back(*lufa);
    wieza->children.push_back(*lacznik);
    kadlub->children.push_back(*wieza);



    //Shader
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    
    apos_location = glGetAttribLocation(program, "aPos");
    aNormals_location = glGetAttribLocation(program, "aNormal");
    texCoord_location = glGetAttribLocation(program, "aTexCoord");
    mvp_location = glGetUniformLocation(program, "MVP");
    mv_location = glGetUniformLocation(program, "MV");
    lightPos_location = glGetUniformLocation(program, "lightPos");

    glUniform3fv(glGetUniformLocation(program, "Ka"), 1, material.Ka);
    glUniform3fv(glGetUniformLocation(program, "Ks"), 1, material.Ks);
    glUniform3fv(glGetUniformLocation(program, "Ke"), 1, material.Ke);
    glUniform1f(glGetUniformLocation(program, "Ns"), material.Ns);
    


    // glEnableVertexAttribArray(apos_location);
    // glVertexAttribPointer(apos_location, 3, GL_FLOAT, GL_FALSE,
    //                     sizeof(CompleteVertex), (void*) 0);

    // glEnableVertexAttribArray(aNormals_location);
    // glVertexAttribPointer(aNormals_location, 3, GL_FLOAT, GL_FALSE,
    //                     sizeof(CompleteVertex), (void*) (sizeof(float) * 3));
    // glEnableVertexAttribArray(texCoord_location);
    // glVertexAttribPointer(texCoord_location, 2, GL_FLOAT, GL_FALSE,
    //                     sizeof(CompleteVertex), (void*) (sizeof(float) * 6));

    vec3 cameraPos = {0.0f,1.0f,4.0f};
    vec3 cameraTarget = {0.0f, 0.0f, 0.0f};
    vec3 up = { 0.0f, 1.0f, 0.0f };
    vec3 lightPosition = {1.0f, 5.0f, 3.0f};

    GLuint texture;
    glGenTextures(1, &texture);

    int width1, height1, channels1;
    unsigned char *img1 = stbi_load(("textures/"+ material.map_Kd).c_str(), &width1, &height1, &channels1, 0);
    if(img1 == NULL) {
        printf("Error in loading the image\n");
    }else{
        printf("Image imported\n");
        GLenum format = (channels1 == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, texture);
    
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, img1);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    stbi_image_free(img1);

    mat4x4 kf1, kf2, kf3, l1,l2,l3;
    mat4x4_identity(kf1);                  
    mat4x4_rotate_Y(kf2, kf1, M_PI / 2.0f);     
    mat4x4_rotate_Y(kf3, kf1, M_PI);           

    kadlub->children[0].AddKeyframe(kf1, 0.0f);              
    kadlub->children[0].AddKeyframe(kf2, 2.0f);              
    kadlub->children[0].AddKeyframe(kf3, 4.0f);

    mat4x4 kf1_lufa, kf2_lufa, kf3_lufa;
    mat4x4_identity(kf1_lufa);              
    mat4x4_translate(kf2_lufa, 0.0f, 0.1f, 0.0f);   
    mat4x4_identity(kf3_lufa);                 

    kadlub->children[0].children[0].AddKeyframe(kf1_lufa, 0.0f);
    kadlub->children[0].children[0].AddKeyframe(kf2_lufa, 1.0f); 
    kadlub->children[0].children[0].AddKeyframe(kf3_lufa, 2.0f);  

    float animationTime = 0.0f;  
    float animationDuration = 4.0f;    

    while (!glfwWindowShouldClose(window))
    {
        animationTime += 0.016f;
        if (animationTime > animationDuration) animationTime -= animationDuration;

        kadlub->InterpolateKeyframes(animationTime);

        float ratio;
        int width, height;
        mat4x4 m, p, mvp,v,vt,lookat,mv;  
        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;
        glViewport(0, 0, width, height);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        vec3 cameraFront;
        cameraFront[0] = cosf(degrees_to_radians(yaw)) * cosf(degrees_to_radians(pitch));
        cameraFront[1] = sinf(degrees_to_radians(pitch));
        cameraFront[2] = sinf(degrees_to_radians(yaw)) * cosf(degrees_to_radians(pitch));
        vec3 cameraTarget = { cameraPos[0] + cameraFront[0], cameraPos[1] + cameraFront[1], cameraPos[2] + cameraFront[2] };
        vec3 zc,xc,yc;
        
        vec3_sub(zc,cameraPos,cameraTarget);
        vec3_norm(zc,zc);
        vec3_mul_cross(xc,zc,up);
        vec3_norm(xc,xc);
        vec3_mul_cross(yc,xc,zc);
        mat4x4_look_at(lookat, cameraPos, cameraTarget, up);
        if(isWKeyPressed){
            if(isLKeyPressed){
                vec3 dir ={0.0f,0.0f,0.1f};
                lightPosition[2] += 0.1f;
            }else
            {
                vec3 dir;
                vec3_scale(dir,zc,-0.1f);
                vec3_add(cameraPos,cameraPos,dir);
            }
        }
        if(isSKeyPressed){
            if(isLKeyPressed){
                vec3 dir ={0.0f,0.0f,-0.1f};
                lightPosition[2] -= 0.1f;
            }else{
                vec3 dir;
                vec3_scale(dir,zc,0.1f);
                vec3_add(cameraPos,cameraPos,dir);
            }
            
        }
        if(isDKeyPressed){
            if(isLKeyPressed){
                vec3 dir ={0.1f,0.0f,0.0f};
                lightPosition[0] += 0.1f;
            }else{
                vec3 dir;
                vec3_scale(dir,xc,-0.1f);
                vec3_add(cameraPos,cameraPos,dir);
            }

        }
        if(isAKeyPressed){
            if(isLKeyPressed){
                vec3 dir ={-0.1f,0.0f,0.0f};
                lightPosition[0] -= 0.1f;
            }else{
                vec3 dir;
                vec3_scale(dir,xc,+0.1f);
                vec3_add(cameraPos,cameraPos,dir);
            }
            
        }
        if(isSpaceKeyPressed){
            vec3 dir = {0.0f,0.1f,0.0f};
            if(isLKeyPressed){
                lightPosition[1] += 0.1f;
            }
            else
                vec3_add(cameraPos,cameraPos,dir);
            
        }
        if(isCtrKeyPressed){
            vec3 dir = {0.0f,-0.1f,0.0f};
            if(isLKeyPressed){
                lightPosition[1] -= 0.1f;
            }
            else
                vec3_add(cameraPos,cameraPos,dir);
        }
        
        float fovRadians = fov * (M_PI / 180.0f);
        mat4x4_perspective(p,fovRadians,ratio,0.1f,1000.0f);
        mat4x4_identity(m);
        
        
        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture); 
        glUniform1i(glGetUniformLocation(program, "texture1"), 0);
        glUniform3fv(lightPos_location, 1, (const GLfloat*) lightPosition);

        //wieza->Draw();
        mat4x4 identity;
        mat4x4_identity(identity);
        kadlub->Draw(p,lookat, identity);
        //lufa->Draw();
        //lacznik->Draw();


        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    delete lufa;
    delete wieza;
    delete kadlub;
    delete lacznik;
    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
