
MS_PRESET_PATH=/home/pi/MS_presets/video
FILE=$MS_PRESET_PATH/video$1.mp4

TITLE="MPlayer$1"
echo $TITLE
if test -f "$FILE"; then
    echo "$FILE exist"
#    mplayer -title video$1 -quiet -nosound -loop 0 -geometry 128x128+0+0 -zoom  -vo x11 $FILE &
    mplayer -really-quiet -nosound -loop 0 -geometry 128x128+0+0 -zoom  -vo x11 $FILE -title $TITLE  &
#    mplayer -really-quiet -nosound -loop 0 -geometry 512x128+0+0 -zoom  -vo x11 $FILE &

else
    echo "###################################################################"
    echo "################ ERROR : Video File $FILE not found !!! ###########"
    echo "###################################################################"
    
fi

#mplayer -geometry 128x128+0+0 -zoom  -cache 128 -tv driver=v4l2:width=640:height=480:fps=25:outfmt=mjpeg:device=/dev/video0 -nosound -vo x11 tv://

#mplayer  -geometry 128x128+0+0 -zoom  rtsp://192.168.1.25:5554 -vo x11

