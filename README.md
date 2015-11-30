# segmentation-sounds
OpenCV fg_bg routine to create OSC-messages for creating sounds. This is based on OpenCV cpp-example-segm_fg_bg,
which I modified to send OSC-messages based on the foreground segmentation. In the original code, a contour is
extracted having the utmost contour, which should approximately describe the object. I used the different properties
(area, bounding box, contour length) to create a varying OSC-message. Area is the gain, bounding box is used for
pitch and panning, and contour length for modulation. The CSD is only something, mainly a proof of concept.

In my quick tests, segmentated hands can change the sound, haven't yet have time for extensive tests, but they're
on my list.

You can't conduct an orchestra, but noise can be done.

OpenCV is quite a big package, but I've run it on Win7, Linux and OS X. Linux and OS X are the most easy to set up
for compiling, Windows on the other hand has prebuilt binaries. OpenCV works on Android and iOS also, so just using 
the phone/tablet camera and csound, that could be the next step for live csound concerts.


