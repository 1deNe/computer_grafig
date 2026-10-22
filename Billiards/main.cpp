// Үстінен көрінетін, фондық музыкасы бар шағын бильярд ойыны.
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <AL/al.h>
#include <AL/alc.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 800;
constexpr float TABLE_LEFT = -0.75f, TABLE_RIGHT = 0.75f;
constexpr float TABLE_BOTTOM = -0.47f, TABLE_TOP = 0.47f;
constexpr float BALL_RADIUS = 0.030f;
constexpr float PI = 3.1415926535f;

const char* vertexSource = R"(
#version 330 core
layout(location=0) in vec2 aPosition;
uniform vec2 uPosition;
uniform vec2 uSize;
out vec2 vLocal;
void main(){ vLocal=aPosition+vec2(0.5); gl_Position=vec4(aPosition*uSize+uPosition,0.0,1.0); }
)";
const char* fragmentSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
uniform vec2 uSize;
uniform float uRadius;
uniform float uCircle;
in vec2 vLocal;
void main(){
    vec2 halfSize=uSize*0.5;
    vec2 point=(vLocal-vec2(0.5))*uSize;
    float radius=uCircle>0.5?min(halfSize.x,halfSize.y):min(uRadius,min(halfSize.x,halfSize.y));
    vec2 q=abs(point)-halfSize+vec2(radius);
    float edgeDistance=uCircle>0.5?length(point)-radius:length(max(q,vec2(0.0)))+min(max(q.x,q.y),0.0)-radius;
    float coverage=1.0-smoothstep(-0.0015,0.0015,edgeDistance);
    if(coverage<0.01)discard;
    FragColor=vec4(uColor,coverage);
}
)";
GLint cornerRadiusUniform=-1;
GLint circleUniform=-1;

enum class Screen { Menu, ModeSelect, Settings, Game };
enum class SettingsTab { Sound, Screen, Gameplay };
enum class GameMode { FreePyramid, CombinedPyramid, DynamicPyramid, ClassicPyramid, Pool8, Pool9, StraightPool, Snooker, SixRedSnooker, CaromOne, CaromThree };
const GameMode visibleModes[]={GameMode::FreePyramid,GameMode::Pool8,GameMode::Snooker,GameMode::CaromThree};
enum class TextId {
    Title, BotGame, FriendGame, Settings, Exit, Back, Language, Sound,
    FpsLimit, ShowFps, BotLevel, Easy, Medium, Hard, Money, Turn, Aim,
    Win, Reward, Table, Buy, Select, Owned, PlayAgain, PlayerOne, PlayerTwo,
    Difficulty, Volume, FreePyramid, CombinedPyramid, DynamicPyramid, ClassicPyramid,
    Pool8, Pool9, StraightPool, Snooker, SixRedSnooker, CaromOne, CaromThree, CallPocket, PlaceCue,
    Resolution, Quality, LowQuality, MediumQuality, HighQuality, Shoot, Power, CancelShot,
    MusicVolume, GameVolume, ScreenTab, GameplayTab, Count
};
int language = 0; // 0: қазақша, 1: орысша, 2: ағылшынша
int graphicsQuality = 1;
float currentAspectRatio = static_cast<float>(WINDOW_WIDTH)/WINDOW_HEIGHT;

const char* text(TextId id) {
    static const char* table[][3] = {
        {"БИЛЬЯРД", "БИЛЬЯРД", "BILLIARDS"},
        {"БОТПЕН ОЙНАУ", "ИГРАТЬ С БОТОМ", "PLAY VS BOT"},
        {"ДОСЫҢМЕН ОЙНАУ", "ИГРАТЬ С ДРУГОМ", "PLAY WITH FRIEND"},
        {"БАПТАУЛАР", "НАСТРОЙКИ", "SETTINGS"},
        {"ШЫҒУ", "ВЫХОД", "EXIT"},
        {"АРТҚА", "НАЗАД", "BACK"},
        {"ТІЛ", "ЯЗЫК", "LANGUAGE"},
        {"ДАУЫС", "ЗВУК", "SOUND"},
        {"FPS ШЕГІ", "ЛИМИТ FPS", "FPS LIMIT"},
        {"FPS КӨРСЕТУ", "ПОКАЗ FPS", "SHOW FPS"},
        {"БОТ ДЕҢГЕЙІ", "СЛОЖНОСТЬ БОТА", "BOT LEVEL"},
        {"ОҢАЙ", "ЛЕГКО", "EASY"},
        {"ОРТАША", "СРЕДНЕ", "MEDIUM"},
        {"ҚИЫН", "СЛОЖНО", "HARD"},
        {"АҚША", "МОНЕТЫ", "COINS"},
        {"КЕЗЕК", "ХОД", "TURN"},
        {"КӨЗДЕП, БАС", "ЦЕЛЬСЯ И БЕЙ", "AIM AND CLICK"},
        {"ЖЕҢІС", "ПОБЕДА", "WIN"},
        {"СЫЙАҚЫ", "НАГРАДА", "REWARD"},
        {"ҮСТЕЛ", "СТОЛ", "TABLE"},
        {"САТЫП АЛУ", "КУПИТЬ", "BUY"},
        {"ТАҢДАУ", "ВЫБРАТЬ", "SELECT"},
        {"АЛЫНДЫ", "КУПЛЕНО", "OWNED"},
        {"ҚАЙТА ОЙНАУ", "ИГРАТЬ СНОВА", "PLAY AGAIN"},
        {"1-ОЙЫНШЫ", "ИГРОК 1", "PLAYER 1"},
        {"2-ОЙЫНШЫ", "ИГРОК 2", "PLAYER 2"},
        {"ҚИЫНДЫҚ", "СЛОЖНОСТЬ", "DIFFICULTY"},
        {"ДАУЫС", "ГРОМКОСТЬ", "VOLUME"},
        {"ЕРКІН ПИРАМИДА", "СВОБОДНАЯ ПИРАМИДА", "FREE PYRAMID"},
        {"ҚҰРАМА ПИРАМИДА", "КОМБИНИРОВАННАЯ", "COMBINED PYRAMID"},
        {"ДИНАМИКА", "ДИНАМИЧЕСКАЯ", "DYNAMIC"},
        {"КЛАССИКА", "КЛАССИЧЕСКАЯ", "CLASSIC"},
        {"ПУЛ-8", "ПУЛ-8", "POOL 8"},
        {"ПУЛ-9", "ПУЛ-9", "POOL 9"},
        {"СТРЕЙТ ПУЛ", "СТРЕЙТ ПУЛ", "STRAIGHT POOL"},
        {"СНУКЕР", "СНУКЕР", "SNOOKER"},
        {"СНУКЕР-6", "СНУКЕР-6", "SIX-RED SNOOKER"},
        {"КАРАМБОЛЬ 1 БОРТ", "КАРАМБОЛЬ 1 БОРТ", "1-CUSHION CAROM"},
        {"КАРАМБОЛЬ 3 БОРТ", "КАРАМБОЛЬ 3 БОРТ", "3-CUSHION CAROM"},
        {"8-ЛУЗАНЫ ТАҢДА (1-6)", "ВЫБЕРИ ЛУЗУ (1-6)", "CALL POCKET (1-6)"},
        {"ОҢ ТҮЙМЕМЕН БИТТІ ОРНАТ", "ПРАВАЯ КНОПКА — ПОСТАВИТЬ ШАР", "RIGHT CLICK TO PLACE CUE"},
        {"АЖЫРАТЫМДЫЛЫҚ", "РАЗРЕШЕНИЕ", "RESOLUTION"},
        {"ГРАФИКА САПАСЫ", "КАЧЕСТВО ГРАФИКИ", "GRAPHICS QUALITY"},
        {"ТӨМЕН", "НИЗКОЕ", "LOW"},
        {"ОРТАША", "СРЕДНЕЕ", "MEDIUM"},
        {"ЖОҒАРЫ", "ВЫСОКОЕ", "HIGH"},
        {"СОҒУ", "УДАРИТЬ", "SHOOT"},
        {"СОҚҚЫ КҮШІ", "СИЛА УДАРА", "SHOT POWER"},
        {"БАС ТАРТУ", "ОТМЕНА", "CANCEL"},
        {"МУЗЫКА ДАУЫСЫ", "ГРОМКОСТЬ МУЗЫКИ", "MUSIC VOLUME"},
        {"ОЙЫН ДАУЫСЫ", "ЗВУК ИГРЫ", "GAME SOUND"},
        {"ЭКРАН", "ЭКРАН", "SCREEN"},
        {"ТІЛ БОТ", "ЯЗЫК БОТ", "LANG BOT"}
    };
    return table[static_cast<int>(id)][language];
}

struct Ball { int number=0; float x=0, y=0, vx=0, vy=0; bool potted=false; };
struct Pocket { float x, y; };
struct Theme { const char* name[3]; float r,g,b; int price; };
const std::array<Theme,4> themes = {{
    {{"ЖАСЫЛ", "ЗЕЛЁНЫЙ", "CLASSIC GREEN"},0.08f,0.39f,0.25f,0},
    {{"КӨК", "СИНИЙ", "OCEAN BLUE"},0.06f,0.28f,0.48f,20},
    {{"ҚЫЗЫЛ", "КРАСНЫЙ", "RUBY RED"},0.48f,0.12f,0.17f,35},
    {{"КҮЛГІН", "ФИОЛЕТОВЫЙ", "PURPLE NIGHT"},0.28f,0.14f,0.43f,55}
}};

GLuint compileShader(GLenum kind, const char* src) {
    GLuint shader=glCreateShader(kind); glShaderSource(shader,1,&src,nullptr); glCompileShader(shader);
    GLint ok=GL_FALSE; glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok){ GLint n=0; glGetShaderiv(shader,GL_INFO_LOG_LENGTH,&n); std::string log(static_cast<size_t>(std::max(n,1)),'\0'); glGetShaderInfoLog(shader,n,nullptr,log.data()); std::cerr<<log<<'\n'; glDeleteShader(shader); return 0; }
    return shader;
}
GLuint makeProgram(){
    GLuint v=compileShader(GL_VERTEX_SHADER,vertexSource), f=compileShader(GL_FRAGMENT_SHADER,fragmentSource);
    if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return 0;}
    GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);
    GLint ok=GL_FALSE;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){glDeleteProgram(p);return 0;}return p;
}
void resize(GLFWwindow*,int w,int h){glViewport(0,0,w,h);}
void rect(GLint pos,GLint size,GLint color,float x,float y,float w,float h,float r,float g,float b){
    glUniform2f(pos,x,y);glUniform2f(size,w,h);glUniform3f(color,r,g,b);
    if(cornerRadiusUniform>=0)glUniform1f(cornerRadiusUniform,0.05f);
    if(circleUniform>=0)glUniform1f(circleUniform,0.0f);
    glDrawArrays(GL_TRIANGLES,0,6);
}
void circle(GLint pos,GLint size,GLint color,float x,float y,float diameter,float r,float g,float b){
    glUniform2f(pos,x,y);glUniform2f(size,diameter,diameter);glUniform3f(color,r,g,b);
    if(cornerRadiusUniform>=0)glUniform1f(cornerRadiusUniform,diameter*0.5f);
    if(circleUniform>=0)glUniform1f(circleUniform,1.0f);
    glDrawArrays(GL_TRIANGLES,0,6);
}

class Sound {
public:
    bool init(){
        device=alcOpenDevice(nullptr); if(!device)return false;
        context=alcCreateContext(device,nullptr); if(!context||!alcMakeContextCurrent(context))return false;
        alGenSources(1,&source); alGenBuffers(static_cast<ALsizei>(buffers.size()),buffers.data());
        const float frequencies[]={620.0f,190.0f,480.0f,760.0f};
        const float durations[]={0.055f,0.075f,0.18f,0.42f};
        for(size_t i=0;i<buffers.size();++i){
            constexpr int rate=22050; const int count=static_cast<int>(rate*durations[i]);
            std::vector<int16_t> pcm(static_cast<size_t>(count));
            for(int n=0;n<count;++n){
                const float t=static_cast<float>(n)/rate;
                const float envelope=std::exp(-t*(i==3?3.0f:18.0f));
                const float sweep=i==1?80.0f:0.0f;
                const float wave=std::sin(2.0f*PI*(frequencies[i]*t+sweep*t*t*0.5f));
                pcm[static_cast<size_t>(n)]=static_cast<int16_t>(wave*envelope*26000.0f);
            }
            alBufferData(buffers[i],AL_FORMAT_MONO16,pcm.data(),static_cast<ALsizei>(pcm.size()*sizeof(int16_t)),rate);
        }
        musicSource=0;musicBuffer=0;alGenSources(1,&musicSource);alGenBuffers(1,&musicBuffer);
        constexpr int rate=22050;constexpr float beat=0.5f;constexpr int beats=16;
        constexpr float melody[beats]={261.63f,329.63f,392.00f,329.63f,293.66f,349.23f,440.00f,349.23f,
                                       220.00f,293.66f,349.23f,293.66f,246.94f,329.63f,392.00f,329.63f};
        constexpr float bass[4]={130.81f,146.83f,110.00f,123.47f};
        std::vector<int16_t> music(static_cast<size_t>(rate*beat*beats));
        for(size_t n=0;n<music.size();++n){
            const float t=static_cast<float>(n)/rate;const int step=std::min(static_cast<int>(t/beat),beats-1);
            const float local=t-step*beat;const float pluck=std::exp(-local*5.0f);
            const float note=std::sin(2.0f*PI*melody[step]*t)+0.22f*std::sin(4.0f*PI*melody[step]*t);
            const float low=std::sin(2.0f*PI*bass[step/4]*t)*0.22f;
            music[n]=static_cast<int16_t>(std::clamp((note*pluck+low)*6200.0f,-32767.0f,32767.0f));
        }
        alBufferData(musicBuffer,AL_FORMAT_MONO16,music.data(),static_cast<ALsizei>(music.size()*sizeof(int16_t)),rate);
        alSourcei(musicSource,AL_BUFFER,static_cast<ALint>(musicBuffer));alSourcei(musicSource,AL_LOOPING,AL_TRUE);
        return true;
    }
    void play(int index,float volume){if(!source||volume<=0.001f)return;alSourceStop(source);alSourcei(source,AL_BUFFER,buffers[static_cast<size_t>(index)]);alSourcef(source,AL_GAIN,volume);alSourcePlay(source);}
    void startMusic(float volume){if(!musicSource)return;alSourcef(musicSource,AL_GAIN,volume*0.22f);alSourcePlay(musicSource);}
    void setMusicVolume(float volume){if(musicSource)alSourcef(musicSource,AL_GAIN,volume*0.22f);}
    void close(){if(source)alDeleteSources(1,&source);if(musicSource)alDeleteSources(1,&musicSource);if(buffers[0])alDeleteBuffers(static_cast<ALsizei>(buffers.size()),buffers.data());if(musicBuffer)alDeleteBuffers(1,&musicBuffer);if(context){alcMakeContextCurrent(nullptr);alcDestroyContext(context);}if(device)alcCloseDevice(device);source=musicSource=0;musicBuffer=0;context=nullptr;device=nullptr;}
private:
    ALCdevice* device=nullptr; ALCcontext* context=nullptr; ALuint source=0,musicSource=0,musicBuffer=0; std::array<ALuint,4> buffers{};
};

// 5x7 пиксель қарпі. Кирилл және қазақ әріптері мәзірлерде қолданылады.
const char* glyph(char32_t c){
    switch(c){
    case U'A':case U'А': return "01110/10001/10001/11111/10001/10001/10001";
    case U'B':case U'В': return "11110/10001/10001/11110/10001/10001/11110";
    case U'C':case U'С': return "01111/10000/10000/10000/10000/10000/01111";
    case U'D': return "11110/10001/10001/10001/10001/10001/11110";
    case U'E':case U'Е':case U'Ё': return "11111/10000/10000/11110/10000/10000/11111";
    case U'F': return "11111/10000/10000/11110/10000/10000/10000";
    case U'G': return "01110/10001/10000/10111/10001/10001/01110";
    case U'H':case U'Н': return "10001/10001/10001/11111/10001/10001/10001";
    case U'I':case U'І': return "11111/00100/00100/00100/00100/00100/11111";
    case U'J': return "00111/00010/00010/00010/10010/10010/01100";
    case U'K':case U'К':case U'Қ': return "10001/10010/10100/11000/10100/10010/10001";
    case U'L': return "10000/10000/10000/10000/10000/10000/11111";
    case U'M':case U'М': return "10001/11011/10101/10101/10001/10001/10001";
    case U'N': return "10001/11001/10101/10011/10001/10001/10001";
    case U'O':case U'О':case U'Ө': return "01110/10001/10001/10001/10001/10001/01110";
    case U'P':case U'Р': return "11110/10001/10001/11110/10000/10000/10000";
    case U'Q': return "01110/10001/10001/10001/10101/10010/01101";
    case U'R': return "11110/10001/10001/11110/10100/10010/10001";
    case U'S': return "01111/10000/10000/01110/00001/00001/11110";
    case U'T':case U'Т': return "11111/00100/00100/00100/00100/00100/00100";
    case U'U':case U'У':case U'Ү':case U'Ұ': return "10001/10001/10001/01111/00001/00010/11100";
    case U'V': return "10001/10001/10001/10001/10001/01010/00100";
    case U'W': return "10001/10001/10001/10101/10101/10101/01010";
    case U'X':case U'Х': return "10001/10001/01010/00100/01010/10001/10001";
    case U'Y': return "10001/10001/01010/00100/00100/00100/00100";
    case U'Z': return "11111/00001/00010/00100/01000/10000/11111";
    case U'Б': return "11111/10000/10000/11110/10001/10001/11110";
    case U'Г': return "11111/10000/10000/10000/10000/10000/10000";
    case U'Ғ': return "11111/10000/10000/11110/10000/10000/10000";
    case U'Д': return "01110/01010/01010/01010/10001/10001/11111";
    case U'Ж': return "10101/10101/01110/00100/01110/10101/10101";
    case U'З': return "11110/00001/00001/01110/00001/00001/11110";
    case U'И':case U'Й': return "10001/10011/10101/10101/11001/10001/10001";
    case U'Л': return "00111/01001/01001/01001/10001/10001/10001";
    case U'Ь':case U'Ъ': return "10000/10000/10000/11110/10001/10001/11110";
    case U'Ч': return "10001/10001/10001/01111/00001/00001/00001";
    case U'Ш':case U'Щ': return "10101/10101/10101/10101/10101/10101/11111";
    case U'Ы': return "10001/10001/10001/11101/10011/10001/11101";
    case U'Э': return "11111/00001/00001/01111/00001/00001/11111";
    case U'Ю': return "10111/10101/10101/11101/10101/10101/10111";
    case U'Я': return "01111/10001/10001/01111/00101/01001/10001";
    case U'П': return "11111/10001/10001/10001/10001/10001/10001";
    case U'Ц': return "10001/10001/10001/10001/10001/10001/11111";
    case U'Ф': return "00100/01110/10101/10101/01110/00100/00100";
    case U'Ң': return "10001/10001/10001/11111/10001/10011/10001";
    case U' ' : return "00000/00000/00000/00000/00000/00000/00000";
    case U'0': return "01110/10001/10011/10101/11001/10001/01110";
    case U'1': return "00100/01100/00100/00100/00100/00100/01110";
    case U'2': return "01110/10001/00001/00010/00100/01000/11111";
    case U'3': return "11110/00001/00001/01110/00001/00001/11110";
    case U'4': return "00010/00110/01010/10010/11111/00010/00010";
    case U'5': return "11111/10000/10000/11110/00001/00001/11110";
    case U'6': return "01110/10000/10000/11110/10001/10001/01110";
    case U'7': return "11111/00001/00010/00100/01000/01000/01000";
    case U'8': return "01110/10001/10001/01110/10001/10001/01110";
    case U'9': return "01110/10001/10001/01111/00001/00001/01110";
    case U':': return "00000/00100/00100/00000/00100/00100/00000";
    case U'+': return "00000/00100/00100/11111/00100/00100/00000";
    case U'-': return "00000/00000/00000/11111/00000/00000/00000";
    case U'%': return "11001/11010/00100/01000/10110/00110/00000";
    default: return "00000/00000/00000/00000/00000/00000/00000";
    }
}

std::vector<char32_t> decodeUtf8(const std::string& str){
    std::vector<char32_t> out;
    for(size_t i=0;i<str.size();){
        const unsigned char b=static_cast<unsigned char>(str[i]);
        if(b<0x80){out.push_back(b);++i;continue;}
        size_t n=1;char32_t cp=0;
        if((b&0xE0)==0xC0&&i+1<str.size()){n=2;cp=b&0x1F;}
        else if((b&0xF0)==0xE0&&i+2<str.size()){n=3;cp=b&0x0F;}
        else if((b&0xF8)==0xF0&&i+3<str.size()){n=4;cp=b&0x07;}
        else{out.push_back(U'?');++i;continue;}
        for(size_t j=1;j<n;++j)cp=(cp<<6)|(static_cast<unsigned char>(str[i+j])&0x3F);
        out.push_back(cp);i+=n;
    }
    return out;
}

void drawText(GLint p,GLint s,GLint c,const std::string& value,float x,float y,float cellY,float r,float g,float b){
    const auto chars=decodeUtf8(value);
    const float cellX=cellY*currentAspectRatio;
    const float advance=cellX*6.0f;
    float cursor=x-advance*static_cast<float>(chars.size())*0.5f;
    for(char32_t ch:chars){
        const std::string pattern=glyph(ch);int row=0,col=0;
        for(char bit:pattern){
            if(bit=='/'){++row;col=0;continue;}
            if(bit=='1')rect(p,s,c,cursor+(col-2.0f)*cellX,y+(3.0f-row)*cellY,cellX*0.82f,cellY*0.82f,r,g,b);
            ++col;
        }
        cursor+=advance;
    }
}

void button(GLint p,GLint s,GLint c,const std::string& label,float x,float y,float w,float h,bool hot=false){
    rect(p,s,c,x,y,w,h,hot?0.18f:0.08f,hot?0.32f:0.19f,hot?0.27f:0.22f);
    rect(p,s,c,x,y-h*0.5f+0.012f,w-0.025f,0.012f,hot?0.90f:0.48f,hot?0.72f:0.38f,0.28f);
    const auto n=decodeUtf8(label).size();
    const float cell=std::min(0.009f,(w-0.10f)/(std::max<size_t>(n,1)*6.0f*currentAspectRatio));
    drawText(p,s,c,label,x,y,cell,0.95f,0.92f,0.79f);
}

bool hitButton(GLFWwindow* window,float x,float y,float w,float h){
    double mx=0,my=0;int ww=1,wh=1;glfwGetCursorPos(window,&mx,&my);glfwGetWindowSize(window,&ww,&wh);
    const float nx=static_cast<float>(2.0*mx/ww-1.0),ny=static_cast<float>(1.0-2.0*my/wh);
    return std::abs(nx-x)<=w*0.5f&&std::abs(ny-y)<=h*0.5f;
}

std::array<Pocket,6> pocketLocations(){return {{{TABLE_LEFT,TABLE_BOTTOM},{0,TABLE_BOTTOM},{TABLE_RIGHT,TABLE_BOTTOM},
                                                  {TABLE_LEFT,TABLE_TOP},{0,TABLE_TOP},{TABLE_RIGHT,TABLE_TOP}}};}

bool isPyramid(GameMode mode){return mode==GameMode::FreePyramid||mode==GameMode::CombinedPyramid||mode==GameMode::DynamicPyramid||mode==GameMode::ClassicPyramid;}
bool isCarom(GameMode mode){return mode==GameMode::CaromOne||mode==GameMode::CaromThree;}
float ballRadius(GameMode mode){return isPyramid(mode)?BALL_RADIUS*1.16f:BALL_RADIUS;}
TextId modeText(GameMode mode){return static_cast<TextId>(static_cast<int>(TextId::FreePyramid)+static_cast<int>(mode));}

void rackBalls(std::vector<Ball>& balls,GameMode mode){
    balls.clear();balls.push_back(Ball{0,-0.48f,0.0f,0.0f,0.0f,false});
    int count=15;
    if(mode==GameMode::Pool9)count=9;
    if(mode==GameMode::Snooker)count=21;
    if(mode==GameMode::SixRedSnooker)count=12;
    if(isCarom(mode))count=2;
    if(isCarom(mode)){
        balls.push_back(Ball{1,0.15f,0.22f,0.0f,0.0f,false});
        balls.push_back(Ball{2,0.25f,-0.20f,0.0f,0.0f,false});
        return;
    }
    const int reds=mode==GameMode::Snooker?15:(mode==GameMode::SixRedSnooker?6:0);
    const float radius=ballRadius(mode);
    int number=1;
    const int rows=static_cast<int>(std::ceil((std::sqrt(8.0f*count+1.0f)-1.0f)*0.5f));
    for(int row=0;row<rows&&number<=count;++row){
        for(int col=0;col<=row;++col){
            if(number>count)break;
            int id=number++;
            const float x=0.27f+row*(radius*1.78f);
            const float y=(col-row*0.5f)*(radius*2.08f);
            balls.push_back(Ball{id,x,y,0.0f,0.0f,false});
        }
    }
    if(reds){
        // Snooker түстері үстелдегі нүктелерге қойылады.
        const float spots[][2]={{-0.05f,0.0f},{0.0f,0.0f},{0.0f,0.34f},{0.0f,-0.34f},{-0.24f,0.0f},{0.39f,0.0f}};
        for(int i=0;i<6;++i)balls.push_back(Ball{16+i,spots[i][0],spots[i][1],0.0f,0.0f,false});
        for(size_t i=1;i<=static_cast<size_t>(reds);++i)balls[i].x-=0.22f;
    }
}

bool anyMoving(const std::vector<Ball>& balls){
    for(const auto& b:balls)if(!b.potted&&b.vx*b.vx+b.vy*b.vy>0.0004f)return true;
    return false;
}

void playShot(Ball& cue,float dx,float dy,float power){
    const float len=std::sqrt(dx*dx+dy*dy);if(len<0.001f)return;
    const float speed=0.38f+std::clamp(power,0.0f,1.0f)*2.35f;
    cue.vx=dx/len*speed;cue.vy=dy/len*speed;
}

void drawBall(GLint p,GLint s,GLint c,const Ball& ball,GameMode mode,int quality){
    if(ball.potted)return;
    static const float colors[16][3]={
        {0.98f,0.96f,0.87f},{0.97f,0.80f,0.12f},{0.12f,0.34f,0.84f},{0.88f,0.17f,0.15f},
        {0.49f,0.19f,0.70f},{0.97f,0.48f,0.11f},{0.12f,0.60f,0.31f},{0.62f,0.18f,0.20f},
        {0.06f,0.06f,0.08f},{0.97f,0.80f,0.12f},{0.12f,0.34f,0.84f},{0.88f,0.17f,0.15f},
        {0.49f,0.19f,0.70f},{0.97f,0.48f,0.11f},{0.12f,0.60f,0.31f},{0.62f,0.18f,0.20f}
    };
    float white[3]={0.97f,0.96f,0.91f};
    float blueCue[3]={0.16f,0.48f,0.78f};
    float snookerRed[3]={0.82f,0.18f,0.22f};
    float snookerColors[6][3]={{0.93f,0.77f,0.16f},{0.18f,0.58f,0.32f},{0.47f,0.25f,0.13f},{0.17f,0.38f,0.84f},{0.89f,0.42f,0.60f},{0.05f,0.05f,0.06f}};
    const float radius=ballRadius(mode);
    const float* color=colors[std::clamp(ball.number,0,15)];
    if(isPyramid(mode))color=(mode==GameMode::CombinedPyramid||mode==GameMode::DynamicPyramid)&&ball.number==0?blueCue:white;
    if(mode==GameMode::Snooker||mode==GameMode::SixRedSnooker){
        if(ball.number<=15)color=snookerRed;else color=snookerColors[std::clamp(ball.number-16,0,5)];
    }
    if(quality>0)circle(p,s,c,ball.x+0.006f,ball.y-0.009f,radius*2.12f,0.015f,0.025f,0.022f);
    circle(p,s,c,ball.x,ball.y,radius*2.0f,color[0],color[1],color[2]);
    if(mode==GameMode::Pool8&&ball.number>=9&&ball.number<=15)rect(p,s,c,ball.x,ball.y,radius*1.75f,radius*0.68f,0.96f,0.94f,0.86f);
    if(ball.number==0)circle(p,s,c,ball.x-radius*0.25f,ball.y+radius*0.30f,radius*0.34f,1.0f,1.0f,1.0f);
    else circle(p,s,c,ball.x-radius*0.22f,ball.y+radius*0.23f,radius*0.34f,0.99f,0.98f,0.91f);
    if(quality>=2)circle(p,s,c,ball.x+radius*0.35f,ball.y-radius*0.34f,radius*0.13f,1.0f,1.0f,0.98f);
}

void drawPocket(GLint p,GLint s,GLint c,float x,float y,bool narrow){
    const float opening=narrow?0.074f:0.086f;
    circle(p,s,c,x,y,opening+0.026f,0.44f,0.26f,0.11f);
    circle(p,s,c,x,y,opening,0.012f,0.020f,0.018f);
    circle(p,s,c,x-0.010f,y+0.010f,opening*0.42f,0.035f,0.050f,0.045f);
}

void resetMatch(std::vector<Ball>& balls,GameMode mode,int& turn,bool& shot,int& potted,bool& foul,
                bool& eight,int& winner,float& botWait){
    rackBalls(balls,mode);turn=0;shot=false;potted=0;foul=false;eight=false;winner=-1;botWait=0.8f;
}

void updateBalls(std::vector<Ball>& balls,float dt,const std::array<Pocket,6>& pockets,
                 GameMode mode,bool& cueFoul,bool& eightPotted,int& pottedCount,int& shotPoints,
                 int& pottedMask,int& firstHit,int& eightPocketIndex,int& caromMask,int& caromRails,Sound& sound,float volume,float& soundCooldown){
    const float radius=ballRadius(mode);
    const float limitX=TABLE_RIGHT-radius,limitY=TABLE_TOP-radius;
    for(size_t i=0;i<balls.size();++i){
        Ball& b=balls[i];if(b.potted)continue;
        b.x+=b.vx*dt;b.y+=b.vy*dt;
        bool pocketed=false;
        const float pocketCapture=isPyramid(mode)?radius+0.0025f:0.057f;
        if(!isCarom(mode))for(size_t pocketIndex=0;pocketIndex<pockets.size();++pocketIndex){const auto& pocket=pockets[pocketIndex];const float dx=b.x-pocket.x,dy=b.y-pocket.y;if(dx*dx+dy*dy<pocketCapture*pocketCapture){pocketed=true;if(mode==GameMode::Pool8&&b.number==8)eightPocketIndex=static_cast<int>(pocketIndex);break;}}
        if(pocketed){
            b.potted=true;b.vx=b.vy=0.0f;
            if(b.number==0)cueFoul=true;else{++pottedCount;if(b.number<31)pottedMask|=1<<b.number;shotPoints+=(mode==GameMode::ClassicPyramid?b.number:(b.number>=16?std::clamp(b.number-14,2,7):1));if((mode==GameMode::Pool9&&b.number==9)||(mode==GameMode::Pool8&&b.number==8))eightPotted=true;}
            sound.play(2,volume);continue;
        }
        if(b.x>limitX){b.x=limitX;b.vx=-std::abs(b.vx);if(b.number==0)++caromRails;if(soundCooldown<=0){sound.play(1,volume*0.45f);soundCooldown=0.07f;}}
        if(b.x<-limitX){b.x=-limitX;b.vx=std::abs(b.vx);if(b.number==0)++caromRails;if(soundCooldown<=0){sound.play(1,volume*0.45f);soundCooldown=0.07f;}}
        if(b.y>limitY){b.y=limitY;b.vy=-std::abs(b.vy);if(b.number==0)++caromRails;if(soundCooldown<=0){sound.play(1,volume*0.45f);soundCooldown=0.07f;}}
        if(b.y<-limitY){b.y=-limitY;b.vy=std::abs(b.vy);if(b.number==0)++caromRails;if(soundCooldown<=0){sound.play(1,volume*0.45f);soundCooldown=0.07f;}}
    }
    for(size_t i=0;i<balls.size();++i){
        Ball& a=balls[i];if(a.potted)continue;
        for(size_t j=i+1;j<balls.size();++j){
            Ball& b=balls[j];if(b.potted)continue;
            float dx=b.x-a.x,dy=b.y-a.y;float d2=dx*dx+dy*dy;const float minD=radius*2.0f;
            if(d2>=minD*minD)continue;
            float d=std::sqrt(std::max(d2,0.000001f));float nx=dx/d,ny=dy/d;
            const float overlap=(minD-d)*0.5f;a.x-=nx*overlap;a.y-=ny*overlap;b.x+=nx*overlap;b.y+=ny*overlap;
            const float relative=(b.vx-a.vx)*nx+(b.vy-a.vy)*ny;
            if(relative<0.0f){if(firstHit<0&&a.number==0)firstHit=b.number;if(firstHit<0&&b.number==0)firstHit=a.number;const float impulse=-(1.94f*relative)*0.5f;a.vx-=impulse*nx;a.vy-=impulse*ny;b.vx+=impulse*nx;b.vy+=impulse*ny;if(soundCooldown<=0){sound.play(1,volume);soundCooldown=0.07f;}}
            if(isCarom(mode)&&relative<0.0f){if(a.number==0)caromMask|=1<<(b.number-1);if(b.number==0)caromMask|=1<<(a.number-1);}
        }
    }
    for(auto& b:balls){
        if(b.potted)continue;
        float speed=std::sqrt(b.vx*b.vx+b.vy*b.vy);
        if(speed<0.018f){b.vx=b.vy=0.0f;continue;}
        const float next=std::max(0.0f,speed-0.62f*dt);const float scale=next/speed;b.vx*=scale;b.vy*=scale;
    }
}

void drawTable(GLint p,GLint s,GLint c,int themeIndex,const std::vector<Ball>& balls,GameMode mode,
               bool aiming,float aimX,float aimY,float now,int calledPocket,int quality){
    rect(p,s,c,0.0f,0.0f,1.93f,1.55f,0.025f,0.035f,0.04f);
    rect(p,s,c,0.0f,0.0f,1.88f,1.50f,0.38f,0.19f,0.08f);
    rect(p,s,c,0.0f,0.0f,1.78f,1.40f,0.72f,0.48f,0.24f);
    const Theme& t=themes[static_cast<size_t>(themeIndex)];
    rect(p,s,c,0.0f,0.0f,1.62f,1.08f,t.r,t.g,t.b);
    rect(p,s,c,0.0f,0.0f,1.50f,0.94f,t.r*0.86f,t.g*0.86f,t.b*0.86f);
    // Көмескі орталық сызық пен гауһар нысаналау белгілері.
    rect(p,s,c,0.0f,0.0f,0.006f,0.92f,0.72f,0.82f,0.72f);
    const int diamondCount=quality==0?5:(quality==1?9:13);
    for(int i=0;i<diamondCount;++i){const float x=-0.64f+i*(1.28f/(diamondCount-1));rect(p,s,c,x,TABLE_TOP+0.11f,0.012f,0.018f,0.88f,0.78f,0.48f);rect(p,s,c,x,TABLE_BOTTOM-0.11f,0.012f,0.018f,0.88f,0.78f,0.48f);}
    if(!isCarom(mode)){const auto pockets=pocketLocations();for(size_t i=0;i<pockets.size();++i){const auto& pk=pockets[i];drawPocket(p,s,c,pk.x,pk.y,isPyramid(mode));if(mode==GameMode::Pool8){const float labelX=std::clamp(pk.x,-0.67f,0.67f),labelY=pk.y<0?TABLE_BOTTOM+0.07f:TABLE_TOP-0.07f;if(static_cast<int>(i)==calledPocket){circle(p,s,c,labelX,labelY,0.042f,0.95f,0.72f,0.25f);circle(p,s,c,labelX,labelY,0.032f,t.r*0.75f,t.g*0.75f,t.b*0.75f);}drawText(p,s,c,std::to_string(i+1),labelX,labelY,0.0045f,0.98f,0.80f,0.38f);}}}
    // Атуға арналған нүктелі бағыт сызығы және ағаш кий.
    if(aiming&&!balls.empty()&&!balls[0].potted){
        const Ball& cue=balls[0];float dx=aimX-cue.x,dy=aimY-cue.y;float len=std::sqrt(dx*dx+dy*dy);
        if(len>0.01f){dx/=len;dy/=len;
            for(float d=0.12f;d<0.84f;d+=0.085f)rect(p,s,c,cue.x+dx*d,cue.y+dy*d,0.012f,0.012f,0.90f,0.91f,0.78f);
            for(float d=0.10f;d<0.43f;d+=0.025f)rect(p,s,c,cue.x-dx*d,cue.y-dy*d,0.018f,0.018f,0.78f,0.49f,0.24f);
        }
    }
    for(const auto& b:balls)drawBall(p,s,c,b,mode,quality);
    (void)now;
}

void loadSave(int& wallet,uint32_t& owned,int& selected,int& lang,int& gameVolume,int& musicVolume,
              int& fpsIndex,int& showFps,int& difficulty,int& resolution,int& quality){
    std::ifstream in("billiards_save.txt");
    if(in){
        in>>wallet>>owned>>selected>>lang>>gameVolume>>fpsIndex>>showFps>>difficulty;
        musicVolume=gameVolume;
        const bool hasVideoSettings=static_cast<bool>(in>>resolution>>quality);
        int savedMusic=musicVolume,savedGame=gameVolume;
        if(in>>savedMusic>>savedGame){musicVolume=savedMusic;gameVolume=savedGame;}
        else if(hasVideoSettings&&in.eof())resolution=resolution==0?1:(resolution==1?3:5);
    }
    wallet=std::max(wallet,0);owned|=1u;selected=std::clamp(selected,0,3);lang=std::clamp(lang,0,2);
    gameVolume=std::clamp(gameVolume,0,10);musicVolume=std::clamp(musicVolume,0,10);fpsIndex=std::clamp(fpsIndex,0,3);showFps=showFps?1:0;difficulty=std::clamp(difficulty,0,2);resolution=std::clamp(resolution,0,6);quality=std::clamp(quality,0,2);
    if((owned&(1u<<selected))==0)selected=0;
}
void saveGame(int wallet,uint32_t owned,int selected,int lang,int gameVolume,int musicVolume,int fpsIndex,int showFps,int difficulty,int resolution,int quality){
    std::ofstream out("billiards_save.txt",std::ios::trunc);
    if(out)out<<wallet<<' '<<owned<<' '<<selected<<' '<<lang<<' '<<gameVolume<<' '<<fpsIndex<<' '<<showFps<<' '<<difficulty<<' '<<resolution<<' '<<quality<<' '<<musicVolume<<' '<<gameVolume<<'\n';
}

} // namespace

int main(){
#ifdef _WIN32
    _putenv_s("ALSOFT_LOGLEVEL","0");
#else
    setenv("ALSOFT_LOGLEVEL","0",1);
#endif
    std::srand(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    int wallet=0,selectedTheme=0,gameVolumeSetting=7,musicVolumeSetting=5,fpsIndex=1,showFps=1,difficulty=1,resolutionIndex=3,qualityIndex=1;
    uint32_t ownedThemes=1u;loadSave(wallet,ownedThemes,selectedTheme,language,gameVolumeSetting,musicVolumeSetting,fpsIndex,showFps,difficulty,resolutionIndex,qualityIndex);
    const int resolutionWidths[]={640,800,1024,1280,1366,1600,1920};
    const int resolutionHeights[]={480,600,768,720,768,900,1080};
    if(!glfwInit()){std::cerr<<"GLFW іске қосылмады.\n";return 1;}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES,4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GLFW_TRUE);
#endif
    GLFWwindow* window=glfwCreateWindow(resolutionWidths[resolutionIndex],resolutionHeights[resolutionIndex],"Billiards",nullptr,nullptr);
    if(!window){glfwTerminate();std::cerr<<"Терезе ашылмады.\n";return 1;}
    currentAspectRatio=static_cast<float>(resolutionWidths[resolutionIndex])/resolutionHeights[resolutionIndex];
    glfwMakeContextCurrent(window);glfwSwapInterval(0);glfwSetFramebufferSizeCallback(window,resize);
    if(!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))){glfwDestroyWindow(window);glfwTerminate();std::cerr<<"OpenGL жүктелмеді.\n";return 1;}
    const GLuint program=makeProgram();if(!program){glfwDestroyWindow(window);glfwTerminate();return 1;}
    constexpr float vertices[]={-0.5f,-0.5f,0.5f,-0.5f,0.5f,0.5f,-0.5f,-0.5f,0.5f,0.5f,-0.5f,0.5f};
    GLuint vao=0,vbo=0;glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),nullptr);glEnableVertexAttribArray(0);
    const GLint pos=glGetUniformLocation(program,"uPosition"),size=glGetUniformLocation(program,"uSize"),color=glGetUniformLocation(program,"uColor");
    cornerRadiusUniform=glGetUniformLocation(program,"uRadius");circleUniform=glGetUniformLocation(program,"uCircle");
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    if(qualityIndex>0)glEnable(GL_MULTISAMPLE);else glDisable(GL_MULTISAMPLE);
    Sound sound;sound.init();sound.startMusic(static_cast<float>(musicVolumeSetting)/10.0f);
    Screen screen=Screen::Menu;bool botMode=true,pendingBot=true;
    GameMode gameMode=GameMode::Pool8;
    int scores[2]={0,0};
    std::vector<Ball> balls;rackBalls(balls,gameMode);
    int currentPlayer=0,pottedThisShot=0,winner=-1;
    int shotPoints=0,caromMask=0,caromRails=0;
    int pottedMask=0,firstHit=-1,targetBall=1,calledPocket=0,eightPocketIndex=-1,poolGroups[2]={0,0},snookerClearTarget=16;
    bool snookerNeedsColor=false,snookerRedsAtStart=false;
    bool shotInProgress=false,cueFoul=false,eightPotted=false,previousMouse=false,cueInHand=false,aimSelected=false;
    float botWait=0.8f,hitSoundCooldown=0.0f;
    const auto pockets=pocketLocations();
    const int fpsOptions[]={30,60,120,0};
    const char* languageNames[]={"ҚАЗАҚША","РУССКИЙ","ENGLISH"};
    const char* resolutionNames[]={"640 x 480 VGA","800 x 600 SVGA","1024 x 768 XGA","1280 x 720 HD (720p)","1366 x 768 HD","1600 x 900 HD+","1920 x 1080 Full HD (1080p)"};
    const TextId qualityNames[]={TextId::LowQuality,TextId::MediumQuality,TextId::HighQuality};
    const TextId settingsTabs[]={TextId::Sound,TextId::ScreenTab,TextId::GameplayTab};
    SettingsTab settingsTab=SettingsTab::Sound;
    graphicsQuality=qualityIndex;
    const char* difficulties[3][3]={{"ОҢАЙ","ЛЕГКО","EASY"},{"ОРТАША","СРЕДНЕ","MEDIUM"},{"ҚИЫН","СЛОЖНО","HARD"}};
    float fps=60.0f,lastFpsTime=0.0f;int fpsFrames=0;
    float aimX=0.0f,aimY=0.0f,shotDx=1.0f,shotDy=0.0f,shotPower=0.45f;

    auto startMatch=[&](bool vsBot,GameMode mode){botMode=vsBot;gameMode=mode;resetMatch(balls,gameMode,currentPlayer,shotInProgress,pottedThisShot,cueFoul,eightPotted,winner,botWait);scores[0]=scores[1]=poolGroups[0]=poolGroups[1]=0;shotPoints=pottedMask=caromMask=caromRails=0;firstHit=-1;targetBall=1;calledPocket=0;eightPocketIndex=-1;snookerClearTarget=16;snookerNeedsColor=snookerRedsAtStart=cueInHand=aimSelected=false;shotPower=0.45f;screen=Screen::Game;};
    auto save=[&](){saveGame(wallet,ownedThemes,selectedTheme,language,gameVolumeSetting,musicVolumeSetting,fpsIndex,showFps,difficulty,resolutionIndex,qualityIndex);};
    auto beginShot=[&](float dx,float dy,float power){
        if(balls.empty()||balls[0].potted)return;
        playShot(balls[0],dx,dy,power);shotInProgress=true;pottedThisShot=0;shotPoints=0;pottedMask=0;firstHit=-1;eightPocketIndex=-1;cueFoul=false;eightPotted=false;caromMask=0;caromRails=0;
        cueInHand=false;
        if(gameMode==GameMode::Pool9){targetBall=10;for(size_t i=1;i<balls.size();++i)if(!balls[i].potted)targetBall=std::min(targetBall,balls[i].number);}
        if(gameMode==GameMode::Snooker||gameMode==GameMode::SixRedSnooker){const int redCount=gameMode==GameMode::Snooker?15:6;snookerRedsAtStart=false;for(size_t i=1;i<=static_cast<size_t>(redCount)&&i<balls.size();++i)if(!balls[i].potted)snookerRedsAtStart=true;}
        sound.play(0,static_cast<float>(gameVolumeSetting)/10.0f);
    };

    auto frameStart=std::chrono::steady_clock::now();
    float previousTime=static_cast<float>(glfwGetTime());
    while(!glfwWindowShouldClose(window)){
        frameStart=std::chrono::steady_clock::now();
        glfwPollEvents();
        const float now=static_cast<float>(glfwGetTime());
        const float dt=std::min(now-previousTime,0.04f);previousTime=now;
        hitSoundCooldown=std::max(0.0f,hitSoundCooldown-dt);
        double mx=0.0,my=0.0;int ww=WINDOW_WIDTH,wh=WINDOW_HEIGHT;glfwGetCursorPos(window,&mx,&my);glfwGetWindowSize(window,&ww,&wh);
        currentAspectRatio=static_cast<float>(ww)/std::max(wh,1);
        const float mouseX=static_cast<float>(2.0*mx/ww-1.0),mouseY=static_cast<float>(1.0-2.0*my/wh);
        if(aimSelected){aimX=balls[0].x+shotDx*0.55f;aimY=balls[0].y+shotDy*0.55f;}
        else{aimX=mouseX;aimY=mouseY;}
        const bool mouseDown=glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS;
        const bool clicked=mouseDown&&!previousMouse;
        const bool rightClicked=glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS;
        const bool escape=glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS;
        if(escape&&screen!=Screen::Menu)screen=Screen::Menu;
        if(screen==Screen::Game&&gameMode==GameMode::Pool8){for(int key=GLFW_KEY_1;key<=GLFW_KEY_6;++key)if(glfwGetKey(window,key)==GLFW_PRESS)calledPocket=key-GLFW_KEY_1;}
        if(screen==Screen::Game&&gameMode==GameMode::DynamicPyramid&&cueInHand&&rightClicked&&mouseX>TABLE_LEFT&&mouseX<TABLE_RIGHT&&mouseY>TABLE_BOTTOM&&mouseY<TABLE_TOP){balls[0].x=mouseX;balls[0].y=mouseY;}

        if(clicked&&screen==Screen::Menu){
            sound.play(0,static_cast<float>(gameVolumeSetting)/10.0f);
            if(hitButton(window,0.0f,0.36f,1.02f,0.15f)){pendingBot=true;screen=Screen::ModeSelect;}
            else if(hitButton(window,0.0f,0.15f,1.02f,0.15f)){pendingBot=false;screen=Screen::ModeSelect;}
            else if(hitButton(window,0.0f,-0.06f,1.02f,0.15f)){settingsTab=SettingsTab::Sound;screen=Screen::Settings;}
            else if(hitButton(window,0.0f,-0.27f,1.02f,0.15f))glfwSetWindowShouldClose(window,GLFW_TRUE);
        }else if(clicked&&screen==Screen::ModeSelect){
            if(hitButton(window,0.0f,-0.52f,0.72f,0.13f))screen=Screen::Menu;
            else for(int i=0;i<4;++i){const int column=i%2,row=i/2;const float x=column==0?-0.44f:0.44f,y=0.16f-row*0.31f;
                if(hitButton(window,x,y,0.78f,0.22f)){startMatch(pendingBot,visibleModes[i]);break;}}
        }else if(clicked&&screen==Screen::Settings){
            if(hitButton(window,-0.53f,0.46f,0.50f,0.13f))settingsTab=SettingsTab::Sound;
            else if(hitButton(window,0.0f,0.46f,0.50f,0.13f))settingsTab=SettingsTab::Screen;
            else if(hitButton(window,0.53f,0.46f,0.50f,0.13f))settingsTab=SettingsTab::Gameplay;
            else if(hitButton(window,0.0f,-0.78f,0.78f,0.13f))screen=Screen::Menu;
            else if(settingsTab==SettingsTab::Sound&&hitButton(window,0.0f,0.12f,1.22f,0.12f))musicVolumeSetting=static_cast<int>(std::lround(std::clamp((mouseX+0.61f)/1.22f,0.0f,1.0f)*10.0f));
            else if(settingsTab==SettingsTab::Sound&&hitButton(window,0.0f,-0.32f,1.22f,0.12f))gameVolumeSetting=static_cast<int>(std::lround(std::clamp((mouseX+0.61f)/1.22f,0.0f,1.0f)*10.0f));
            else if(settingsTab==SettingsTab::Screen&&hitButton(window,0.0f,0.20f,1.44f,0.15f)){resolutionIndex=(resolutionIndex+1)%7;glfwSetWindowSize(window,resolutionWidths[resolutionIndex],resolutionHeights[resolutionIndex]);}
            else if(settingsTab==SettingsTab::Screen&&hitButton(window,0.0f,-0.04f,1.44f,0.15f))fpsIndex=(fpsIndex+1)%4;
            else if(settingsTab==SettingsTab::Screen&&hitButton(window,0.0f,-0.28f,1.44f,0.15f)){qualityIndex=(qualityIndex+1)%3;graphicsQuality=qualityIndex;if(qualityIndex==0)glDisable(GL_MULTISAMPLE);else glEnable(GL_MULTISAMPLE);}
            else if(settingsTab==SettingsTab::Screen&&hitButton(window,0.0f,-0.52f,1.44f,0.15f))showFps=1-showFps;
            else if(settingsTab==SettingsTab::Gameplay&&hitButton(window,0.0f,0.18f,1.44f,0.15f))language=(language+1)%3;
            else if(settingsTab==SettingsTab::Gameplay&&hitButton(window,0.0f,-0.12f,1.44f,0.15f))difficulty=(difficulty+1)%3;
            sound.setMusicVolume(static_cast<float>(musicVolumeSetting)/10.0f);
            save();
        }else if(clicked&&screen==Screen::Game&&winner>=0&&hitButton(window,0.0f,-0.15f,0.78f,0.12f)){
            startMatch(botMode,gameMode);
        }else if(clicked&&screen==Screen::Game&&winner<0&&!shotInProgress&&aimSelected&&(!botMode||currentPlayer==0)){
            if(hitButton(window,0.0f,-0.87f,0.54f,0.12f)){
                beginShot(shotDx,shotDy,shotPower);aimSelected=false;
            }else if(hitButton(window,-0.53f,-0.87f,0.34f,0.12f))aimSelected=false;
            else if(hitButton(window,0.0f,-0.70f,0.82f,0.12f))shotPower=std::clamp((mouseX+0.41f)/0.82f,0.05f,1.0f);
            else if(mouseX>TABLE_LEFT&&mouseX<TABLE_RIGHT&&mouseY>TABLE_BOTTOM&&mouseY<TABLE_TOP){
                const float dx=mouseX-balls[0].x,dy=mouseY-balls[0].y,len=std::sqrt(dx*dx+dy*dy);
                if(len>0.025f){shotDx=dx/len;shotDy=dy/len;}
            }
        }else if(clicked&&screen==Screen::Game&&winner<0&&!shotInProgress&&(!botMode||currentPlayer==0)&&
                 mouseX>TABLE_LEFT&&mouseX<TABLE_RIGHT&&mouseY>TABLE_BOTTOM&&mouseY<TABLE_TOP){
            const float dx=mouseX-balls[0].x,dy=mouseY-balls[0].y;
            const float distance=std::sqrt(dx*dx+dy*dy);
            if(distance>0.025f){shotDx=dx/distance;shotDy=dy/distance;shotPower=0.45f;aimSelected=true;}
        }
        previousMouse=mouseDown;

        if(screen==Screen::Game&&winner<0){
            if(shotInProgress){
                updateBalls(balls,dt,pockets,gameMode,cueFoul,eightPotted,pottedThisShot,shotPoints,pottedMask,firstHit,eightPocketIndex,caromMask,caromRails,sound,static_cast<float>(gameVolumeSetting)/10.0f,hitSoundCooldown);
                if(!anyMoving(balls)){
                    if(gameMode==GameMode::Pool8){
                        if(poolGroups[currentPlayer]==0){for(int n=1;n<=15;++n)if(n!=8&&(pottedMask&(1<<n))){poolGroups[currentPlayer]=n<=7?1:2;poolGroups[1-currentPlayer]=3-poolGroups[currentPlayer];break;}}
                        int ownLeft=0;for(size_t i=1;i<balls.size();++i)if(!balls[i].potted&&((poolGroups[currentPlayer]==1&&balls[i].number<=7)||(poolGroups[currentPlayer]==2&&balls[i].number>=9&&balls[i].number<=15)))++ownLeft;
                        if(firstHit<0||(poolGroups[currentPlayer]==0&&firstHit==8)||(poolGroups[currentPlayer]!=0&&((firstHit==8&&ownLeft>0)||(firstHit!=8&&((poolGroups[currentPlayer]==1&&firstHit>7)||(poolGroups[currentPlayer]==2&&firstHit<9))))))cueFoul=true;
                    }
                    if(gameMode==GameMode::Pool9&&firstHit!=targetBall)cueFoul=true;
                    const bool snookerMode=gameMode==GameMode::Snooker||gameMode==GameMode::SixRedSnooker;
                    if(snookerMode){
                        const int redCount=gameMode==GameMode::Snooker?15:6;
                        const bool legalFirst=snookerRedsAtStart?(snookerNeedsColor?(firstHit>=16&&firstHit<=21):(firstHit>=1&&firstHit<=redCount)):(firstHit==snookerClearTarget);
                        if(!legalFirst)cueFoul=true;
                        for(int n=1;n<=21;++n)if(pottedMask&(1<<n)){
                            const bool targetBallPotted=snookerRedsAtStart?(snookerNeedsColor?n==firstHit:n<=redCount):(n==snookerClearTarget);
                            if(!targetBallPotted)cueFoul=true;
                        }
                    }
                    if(cueFoul&&gameMode==GameMode::CombinedPyramid){for(size_t i=balls.size();i>1;--i)if(!balls[i-1].potted){balls[i-1].potted=true;++pottedThisShot;break;}}
                    if(cueFoul){balls[0].potted=false;balls[0].x=-0.48f;balls[0].y=0.0f;balls[0].vx=balls[0].vy=0.0f;cueInHand=gameMode==GameMode::DynamicPyramid;}
                    bool keepTurn=pottedThisShot>0;
                    if(isCarom(gameMode)){
                        const int railsNeeded=gameMode==GameMode::CaromThree?3:1;
                        if(caromMask==3&&caromRails>=railsNeeded){++scores[currentPlayer];keepTurn=true;}
                        else keepTurn=false;
                        if(scores[currentPlayer]>=10)winner=currentPlayer;
                    }else if(gameMode==GameMode::Pool8){
                        if(eightPotted){int groupLeft=0;for(size_t i=1;i<balls.size();++i)if(!balls[i].potted&&((poolGroups[currentPlayer]==1&&balls[i].number<=7)||(poolGroups[currentPlayer]==2&&balls[i].number>=9&&balls[i].number<=15)))++groupLeft;winner=(!cueFoul&&poolGroups[currentPlayer]!=0&&groupLeft==0&&eightPocketIndex==calledPocket)?currentPlayer:1-currentPlayer;keepTurn=false;}
                        else if(poolGroups[currentPlayer]!=0){keepTurn=false;for(int n=1;n<=15;++n)if((pottedMask&(1<<n))&&((poolGroups[currentPlayer]==1&&n<=7)||(poolGroups[currentPlayer]==2&&n>=9)))keepTurn=true;}
                    }else if(gameMode==GameMode::Pool9&&eightPotted){
                        winner=(!cueFoul&&firstHit==targetBall)?currentPlayer:-1;
                        if(winner<0){for(auto& b:balls)if(b.number==9){b.potted=false;b.x=0.35f;b.y=0.0f;break;}}
                    }else if(isPyramid(gameMode)){
                        scores[currentPlayer]+=pottedThisShot;
                        if(scores[currentPlayer]>=8)winner=currentPlayer;
                        if(cueFoul&&gameMode!=GameMode::FreePyramid)keepTurn=true;
                    }else if(gameMode==GameMode::ClassicPyramid){
                        scores[currentPlayer]+=shotPoints;if(scores[currentPlayer]>=71)winner=currentPlayer;
                    }else if(gameMode==GameMode::StraightPool){
                        scores[currentPlayer]+=pottedThisShot;if(scores[currentPlayer]>=100)winner=currentPlayer;
                        int remaining=0;for(size_t i=1;i<balls.size();++i)if(!balls[i].potted)++remaining;
                        if(remaining<=1&&winner<0)rackBalls(balls,gameMode);
                    }else if(gameMode==GameMode::Snooker||gameMode==GameMode::SixRedSnooker){
                        if(cueFoul){scores[1-currentPlayer]+=std::max(4,std::min(7,firstHit>=16?firstHit-14:4));keepTurn=false;}
                        else if(pottedThisShot>0){scores[currentPlayer]+=shotPoints;keepTurn=true;
                            if(snookerRedsAtStart){
                                if(snookerNeedsColor){const int spot=firstHit-16;const float positions[6][2]={{-0.05f,0.0f},{0.0f,0.34f},{0.0f,-0.34f},{-0.24f,0.0f},{0.39f,0.0f},{0.50f,0.0f}};for(auto& b:balls)if(b.number==firstHit){b.potted=false;b.x=positions[std::clamp(spot,0,5)][0];b.y=positions[std::clamp(spot,0,5)][1];break;}snookerNeedsColor=false;}
                                else snookerNeedsColor=true;
                            }else ++snookerClearTarget;
                        }
                        const int reds=gameMode==GameMode::Snooker?15:6;bool redsLeft=false;
                        for(size_t i=1;i<=static_cast<size_t>(reds)&&i<balls.size();++i)if(!balls[i].potted)redsLeft=true;
                        if(!redsLeft&&snookerClearTarget>21)winner=scores[0]>=scores[1]?0:1;
                    }
                    if(winner>=0){sound.play(3,static_cast<float>(gameVolumeSetting)/10.0f);save();}
                    else {if(cueFoul&&gameMode!=GameMode::CombinedPyramid&&gameMode!=GameMode::DynamicPyramid)keepTurn=false;if(!keepTurn)currentPlayer=1-currentPlayer;botWait=0.75f;}
                    shotInProgress=false;
                }
            }else if(botMode&&currentPlayer==1){
                botWait-=dt;
                if(botWait<=0.0f){
                    int target=-1;float best=1e9f;bool otherBalls=false;int pool9Target=10;
                    if(gameMode==GameMode::Pool9)for(size_t i=1;i<balls.size();++i)if(!balls[i].potted)pool9Target=std::min(pool9Target,balls[i].number);
                    for(size_t i=1;i<balls.size();++i)if(!balls[i].potted&&balls[i].number!=8)otherBalls=true;
                    for(size_t i=1;i<balls.size();++i){if(balls[i].potted||(gameMode==GameMode::Pool8&&otherBalls&&balls[i].number==8)||(gameMode==GameMode::Pool9&&balls[i].number!=pool9Target))continue;
                        const float dx=balls[i].x-balls[0].x,dy=balls[i].y-balls[0].y;const float d=dx*dx+dy*dy;if(d<best){best=d;target=static_cast<int>(i);}}
                    if(target>=0){
                        const Ball& obj=balls[static_cast<size_t>(target)];float dx=obj.x-balls[0].x,dy=obj.y-balls[0].y;
                        if(difficulty>0){
                            const Pocket* selected=&pockets[0];float pd=1e9f;int selectedPocket=0;
                            for(size_t pi=0;pi<pockets.size();++pi){const auto& pk=pockets[pi];const float qx=pk.x-obj.x,qy=pk.y-obj.y,d=qx*qx+qy*qy;if(d<pd){pd=d;selected=&pk;selectedPocket=static_cast<int>(pi);}}
                            if(gameMode==GameMode::Pool8&&obj.number==8)calledPocket=selectedPocket;
                            const float len=std::sqrt(pd);if(len>0.001f){const float ox=(obj.x-selected->x)/len,oy=(obj.y-selected->y)/len;dx=(obj.x-ox*ballRadius(gameMode)*2.1f)-balls[0].x;dy=(obj.y-oy*ballRadius(gameMode)*2.1f)-balls[0].y;}
                        }
                        if(gameMode==GameMode::Pool8&&obj.number==8){float pd=1e9f;for(size_t pi=0;pi<pockets.size();++pi){const float qx=pockets[pi].x-obj.x,qy=pockets[pi].y-obj.y,d=qx*qx+qy*qy;if(d<pd){pd=d;calledPocket=static_cast<int>(pi);}}}
                        const float noise=difficulty==0?0.24f:(difficulty==1?0.10f:0.025f);
                        const float angle=std::atan2(dy,dx)+(static_cast<float>(std::rand()%2001)/1000.0f-1.0f)*noise;
                        const float strength=difficulty==0?0.46f:(difficulty==1?0.67f:0.88f);
                        beginShot(std::cos(angle),std::sin(angle),strength);
                    }else{winner=0;save();}
                }
            }
        }

        int cap=fpsOptions[fpsIndex];
        if(screen==Screen::Game){
            glClearColor(0.035f,0.055f,0.07f,1.0f);glClear(GL_COLOR_BUFFER_BIT);glUseProgram(program);
            drawTable(pos,size,color,selectedTheme,balls,gameMode,winner<0&&!shotInProgress&&(!botMode||currentPlayer==0),aimX,aimY,now,calledPocket,graphicsQuality);
            rect(pos,size,color,0.0f,0.83f,0.66f,0.11f,0.06f,0.12f,0.15f);
            const std::string turnLabel=std::string(text(TextId::Turn))+": "+text(currentPlayer==0?TextId::PlayerOne:TextId::PlayerTwo);
            drawText(pos,size,color,turnLabel,0.0f,0.83f,0.0068f,0.95f,0.89f,0.70f);
            drawText(pos,size,color,text(modeText(gameMode)),0.0f,0.70f,0.0058f,0.79f,0.89f,0.80f);
            const std::string scoreLabel=std::to_string(scores[0])+" : "+std::to_string(scores[1]);
            drawText(pos,size,color,scoreLabel,0.0f,0.59f,0.0062f,1.0f,0.83f,0.40f);
            if(gameMode==GameMode::Pool8)drawText(pos,size,color,std::string(text(TextId::CallPocket))+": "+std::to_string(calledPocket+1),0.0f,0.50f,0.0053f,0.85f,0.89f,0.78f);
            if(gameMode==GameMode::DynamicPyramid&&cueInHand)drawText(pos,size,color,text(TextId::PlaceCue),0.0f,0.50f,0.0050f,0.85f,0.89f,0.78f);
            if(aimSelected){
                drawText(pos,size,color,std::string(text(TextId::Power))+": "+std::to_string(static_cast<int>(shotPower*100.0f))+"%",0.0f,-0.60f,0.0060f,1.0f,0.88f,0.68f);
                rect(pos,size,color,0.0f,-0.70f,0.82f,0.055f,0.09f,0.16f,0.16f);
                rect(pos,size,color,-0.41f+0.82f*shotPower*0.5f,-0.70f,0.82f*shotPower,0.035f,0.83f,0.57f,0.23f);
                circle(pos,size,color,-0.41f+0.82f*shotPower,-0.70f,0.075f,0.98f,0.81f,0.43f);
                button(pos,size,color,text(TextId::CancelShot),-0.53f,-0.87f,0.34f,0.12f);
                button(pos,size,color,text(TextId::Shoot),0.0f,-0.87f,0.54f,0.12f,true);
            }
            if(showFps)drawText(pos,size,color,"FPS "+std::to_string(static_cast<int>(fps)),0.78f,0.78f,0.0058f,0.75f,0.83f,0.82f);
            if(winner>=0){
                rect(pos,size,color,0.0f,0.0f,0.92f,0.38f,0.04f,0.08f,0.10f);
                const std::string winnerName=winner==0?text(TextId::PlayerOne):text(botMode?TextId::PlayerTwo:TextId::PlayerTwo);
                drawText(pos,size,color,std::string(text(TextId::Win))+" - "+winnerName,0.0f,0.11f,0.009f,1.0f,0.82f,0.28f);
                button(pos,size,color,text(TextId::PlayAgain),0.0f,-0.15f,0.78f,0.12f,true);
            }
        }else{
            glClearColor(0.025f,0.055f,0.07f,1.0f);glClear(GL_COLOR_BUFFER_BIT);glUseProgram(program);
            // Жылы реңкті айналмалы нүктелер фонға тереңдік береді.
            for(int i=0;i<9;++i){
                const float a=now*0.16f+i*(2.0f*PI/9.0f);
                const float x=std::cos(a)*0.86f,y=std::sin(a*1.35f)*0.67f;
                circle(pos,size,color,x,y,0.13f,0.10f,0.20f,0.18f);
                circle(pos,size,color,x,y,0.052f,0.28f,0.36f,0.29f);
            }
            const float cardWidth=screen==Screen::Settings?1.72f:(screen==Screen::ModeSelect?1.72f:1.24f);
            const float cardHeight=screen==Screen::Settings?1.96f:1.72f;
            rect(pos,size,color,0.0f,-0.015f,cardWidth+0.04f,cardHeight,0.045f,0.075f,0.085f);
            rect(pos,size,color,0.0f,-0.005f,cardWidth,cardHeight-0.04f,0.055f,0.13f,0.13f);
            if(screen==Screen::Settings){
                circle(pos,size,color,0.0f,0.91f,0.09f,0.82f,0.56f,0.22f);
                circle(pos,size,color,0.0f,0.91f,0.058f,0.04f,0.045f,0.04f);
                drawText(pos,size,color,text(TextId::Title),0.0f,0.78f,0.014f,0.96f,0.82f,0.46f);
            }else{
                circle(pos,size,color,0.0f,0.84f,0.12f,0.82f,0.56f,0.22f);
                circle(pos,size,color,0.0f,0.84f,0.078f,0.04f,0.045f,0.04f);
                drawText(pos,size,color,text(TextId::Title),0.0f,0.69f,0.018f,0.96f,0.82f,0.46f);
                rect(pos,size,color,0.0f,0.59f,0.30f,0.006f,0.55f,0.39f,0.20f);
                rect(pos,size,color,0.0f,0.59f,0.055f,0.018f,0.92f,0.73f,0.35f);
            }
            if(screen==Screen::Menu){
                button(pos,size,color,text(TextId::BotGame),0.0f,0.36f,1.02f,0.15f,hitButton(window,0.0f,0.36f,1.02f,0.15f));
                button(pos,size,color,text(TextId::FriendGame),0.0f,0.15f,1.02f,0.15f,hitButton(window,0.0f,0.15f,1.02f,0.15f));
                button(pos,size,color,text(TextId::Settings),0.0f,-0.06f,1.02f,0.15f,hitButton(window,0.0f,-0.06f,1.02f,0.15f));
                button(pos,size,color,text(TextId::Exit),0.0f,-0.27f,1.02f,0.15f,hitButton(window,0.0f,-0.27f,1.02f,0.15f));
            }else if(screen==Screen::ModeSelect){
                for(int i=0;i<4;++i){
                    const int column=i%2,row=i/2;const float x=column==0?-0.44f:0.44f,y=0.16f-row*0.31f;
                    button(pos,size,color,text(modeText(visibleModes[i])),x,y,0.78f,0.22f,hitButton(window,x,y,0.78f,0.22f));
                }
                button(pos,size,color,text(TextId::Back),0.0f,-0.52f,0.72f,0.13f,hitButton(window,0.0f,-0.52f,0.72f,0.13f));
            }else if(screen==Screen::Settings){
                for(int i=0;i<3;++i){const float x=-0.53f+i*0.53f;const bool active=static_cast<int>(settingsTab)==i;button(pos,size,color,text(settingsTabs[i]),x,0.46f,0.50f,0.13f,active||hitButton(window,x,0.46f,0.50f,0.13f));}
                if(settingsTab==SettingsTab::Sound){
                    drawText(pos,size,color,std::string(text(TextId::MusicVolume))+": "+std::to_string(musicVolumeSetting*10)+"%",0.0f,0.27f,0.0062f,1.0f,0.89f,0.70f);
                    rect(pos,size,color,0.0f,0.12f,1.22f,0.045f,0.10f,0.19f,0.18f);rect(pos,size,color,-0.61f+1.22f*musicVolumeSetting/20.0f,0.12f,1.22f*musicVolumeSetting/10.0f,0.028f,0.86f,0.61f,0.27f);circle(pos,size,color,-0.61f+1.22f*musicVolumeSetting/10.0f,0.12f,0.065f,0.98f,0.83f,0.48f);
                    drawText(pos,size,color,std::string(text(TextId::GameVolume))+": "+std::to_string(gameVolumeSetting*10)+"%",0.0f,-0.17f,0.0062f,1.0f,0.89f,0.70f);
                    rect(pos,size,color,0.0f,-0.32f,1.22f,0.045f,0.10f,0.19f,0.18f);rect(pos,size,color,-0.61f+1.22f*gameVolumeSetting/20.0f,-0.32f,1.22f*gameVolumeSetting/10.0f,0.028f,0.86f,0.61f,0.27f);circle(pos,size,color,-0.61f+1.22f*gameVolumeSetting/10.0f,-0.32f,0.065f,0.98f,0.83f,0.48f);
                }else if(settingsTab==SettingsTab::Screen){
                    button(pos,size,color,std::string(text(TextId::Resolution))+": "+resolutionNames[resolutionIndex],0.0f,0.20f,1.44f,0.15f,hitButton(window,0.0f,0.20f,1.44f,0.15f));
                    const std::string fpsLabel=fpsOptions[fpsIndex]==0?"MAX":std::to_string(fpsOptions[fpsIndex]);button(pos,size,color,std::string(text(TextId::FpsLimit))+": "+fpsLabel,0.0f,-0.04f,1.44f,0.15f,hitButton(window,0.0f,-0.04f,1.44f,0.15f));
                    button(pos,size,color,std::string(text(TextId::Quality))+": "+text(qualityNames[qualityIndex]),0.0f,-0.28f,1.44f,0.15f,hitButton(window,0.0f,-0.28f,1.44f,0.15f));
                    button(pos,size,color,std::string(text(TextId::ShowFps))+": "+(showFps?"ON":"OFF"),0.0f,-0.52f,1.44f,0.15f,hitButton(window,0.0f,-0.52f,1.44f,0.15f));
                }else{
                    button(pos,size,color,std::string(text(TextId::Language))+": "+languageNames[language],0.0f,0.18f,1.44f,0.15f,hitButton(window,0.0f,0.18f,1.44f,0.15f));
                    button(pos,size,color,std::string(text(TextId::BotLevel))+": "+difficulties[difficulty][language],0.0f,-0.12f,1.44f,0.15f,hitButton(window,0.0f,-0.12f,1.44f,0.15f));
                }
                button(pos,size,color,text(TextId::Back),0.0f,-0.78f,0.78f,0.13f,hitButton(window,0.0f,-0.78f,0.78f,0.13f));
            }
        }
        glfwSwapBuffers(window);
        ++fpsFrames;if(now-lastFpsTime>=0.5f){fps=static_cast<float>(fpsFrames)/(now-lastFpsTime);fpsFrames=0;lastFpsTime=now;}
        if(cap>0){const auto elapsed=std::chrono::steady_clock::now()-frameStart;const auto target=std::chrono::duration<double>(1.0/static_cast<double>(cap));if(elapsed<target)std::this_thread::sleep_for(target-elapsed);}
    }

    save();sound.close();glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteProgram(program);glfwDestroyWindow(window);glfwTerminate();
    return 0;
}


