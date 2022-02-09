# launch peek
nohup peek &
sleep 0.5
# grab window id
peekID=$(wmctrl -l | tail -n 1)
# launch GUI
nohup ./app/spatial-model-editor &
sleep 0.5
# grab window id
guiID=$(wmctrl -l | tail -n 1)
sleep 0.5
# move and resize GUI
wmctrl -ir $guiID -e 0,100,100,1200,800
sleep 0.5
# move and resize peek over the top
wmctrl -ir $peekID -e 0,94,20,1212,886
