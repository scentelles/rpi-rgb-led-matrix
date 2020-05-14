MS_PRESET_PATH=/home/pi/MS_presets/slideshow
FILE=$MS_PRESET_PATH/slide$1.odp

if test -f "$FILE"; then
    echo "$FILE exist"
    loimpress --show --nologo --minimized $FILE

else
    echo "###################################################################"
    echo "################ ERROR : Slide File $FILE not found !!! ###########"
    echo "###################################################################"
    
fi

