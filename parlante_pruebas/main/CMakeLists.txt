idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES audio_controler
                    EMBED_FILES "../audios/victory8bit.pcm"
                                "../audios/8bit.pcm"
                                "../audios/start.pcm"
                                "../audios/lose.pcm"
                                "../audios/whistle.pcm")
