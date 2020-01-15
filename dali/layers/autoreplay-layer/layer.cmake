SET( LAYER_SOURCES
     ${LAYERS_ROOT}/autoreplay-layer/autoreplay-layer.cpp
     ${LAYERS_ROOT}/autoreplay-layer/autoreplay-layer.h
     ${LAYERS_ROOT}/autoreplay-layer/multi-point-event-serializer.h
     ${LAYERS_ROOT}/autoreplay-layer/touch-point-feed.cpp
     ${LAYERS_ROOT}/autoreplay-layer/replayer.cpp
     ${LAYERS_ROOT}/autoreplay-layer/replayer.h
     ${LAYERS_ROOT}/autoreplay-layer/recorder.cpp
     ${LAYERS_ROOT}/autoreplay-layer/recorder.h
     ${LAYERS_ROOT}/autoreplay-layer/lodepng.cpp
     ${LAYERS_ROOT}/autoreplay-layer/lodepng.h
     ${LAYERS_ROOT}/autoreplay-layer/event-sync-timer.cpp
     ${LAYERS_ROOT}/autoreplay-layer/event-sync-timer.h
     ${LAYERS_ROOT}/autoreplay-layer/picojson.h)

SET( LAYER_NAME autoreplay-layer )

SET( LAYER_INSTALL_FILES ${LAYERS_ROOT}/autoreplay-layer/DALI_autoreplay_layer.json )