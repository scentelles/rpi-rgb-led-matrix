
MS_PRESET_PATH=/home/pi/MS_presets/video
FILE=$MS_PRESET_PATH/video$1.mp4

if test -f "$FILE"; then
    echo "$FILE exist"
#    mplayer -title video$1 -quiet -nosound -loop 0 -geometry 128x128+0+0 -zoom  -vo x11 $FILE &
    mplayer -really-quiet -nosound -loop 0 -geometry 128x128+0+0 -zoom  -vo x11 $FILE &

else
    echo "###################################################################"
    echo "################ ERROR : Video File $FILE not found !!! ###########"
    echo "###################################################################"
    
fi

