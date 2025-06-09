# fast_lio_E1R
This repository fixes the issue of the initial coordinate system camera_init deflection in fast_lio caused by the installation deviation between the robosense e1r LiDAR and the IMU. The problem is solved by rotating the coordinate system of the IMU messages published in the rslidar_sdk driver package.
