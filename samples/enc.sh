#MARKERS=`cat footsteps.txt | xargs`
MARKERS=`cat footsteps.txt`
oggenc -q-1 -c "MARKERS=$MARKERS" footsteps.wav
oggenc -q-1 forest.wav