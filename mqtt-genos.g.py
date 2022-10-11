from signal import siginterrupt
import licant

licant.include("genos")

licant.module("mqtt-genos",
              sources=["src/MQTTPacket/*.c"],
              include_paths=["src"],
              )
