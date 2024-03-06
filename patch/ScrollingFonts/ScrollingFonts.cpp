#include "daisy_patch.h"
#include <string>

using namespace daisy;

DaisyPatch hw;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = 0.f;
        out[1][i] = 0.f;
        out[2][i] = 0.f;
        out[3][i] = 0.f;
    }
}

// Main -- Init, and Midi Handling
int main(void)
{
    // Init
    hw.Init();

    FontDef* fonts[] = {&Font_4x6, &Font_4x8, &Font_5x8, &Font_6x7, &Font_6x8, &Font_7x10, &Font_11x18, &Font_16x26};
    std::string names[] = {"Font_4x6", "Font_4x8", "Font_5x8", "Font_6x7", "Font_6x8", "Font_7x10", "Font_11x18", "16x26"};

    //display
    std::string str  = " A B C D E F G H I J K L M N O P Q R S T U V W X Y Z \
     a b c d e f g h i j k l m n o p q r s t u v w x y z \
     0 1 2 3 4 5 6 7 8 9 \
     ! \" # $ % & ' () * + , - . / : ; \
     < = > ? @ [ \\ ] ^ _ ` { | } ~     ";

    // Start stuff.
    hw.StartAudio(AudioCallback);

    size_t x = 0;
    size_t font = 7;

    for(;;)
    {
        if(x == 0)
            font = (font + 1) % 8;

        hw.display.Fill(false);

        char* cstr = &str[x];
        hw.display.SetCursor(0, 0);
        hw.display.WriteString(cstr, *fonts[font], true);

        cstr = &names[font][0];
        hw.display.SetCursor(0, 32);
        hw.display.WriteString(cstr, *fonts[font], true);

        hw.display.Update();

        if(x == 0)
            System::Delay(1000);

        System::Delay(150);

        x = (x + 1) % strlen(&str[0]);
    }
}
