# SLIC Superpixels from Scratch (C++ / OpenCV)

An implementation of **SLIC** (Simple Linear Iterative Clustering) superpixel segmentation, written in C++ with OpenCV for the Image Processing course at the Technical University of Cluj-Napoca. The clustering is implemented by hand; OpenCV is used only for image I/O, colour conversion and display.

## Algorithm

1. Convert the image to the **CIELAB** colour space.
2. Place `K` cluster centres on a regular grid with spacing `S = sqrt(N / K)`, and move each one to the lowest-gradient position in its 3x3 neighbourhood (Sobel gradient magnitude) to avoid edges and noise.
3. Repeat for 50 iterations:
   - **Assign** each pixel to the nearest centre within a local search window. The distance combines the colour distance in Lab and the spatial distance, weighted by the compactness factor `m`.
   - **Update** each centre to the mean colour and position of its pixels.
4. Colour each superpixel with the mean colour of its pixels and draw the boundaries between different labels.

Default parameters in `main()`: `K = 100` superpixels, compactness `m = 20`, input `Data_Set/man.jpg`.

## Output

Three windows are shown: the original image, the superpixel-filled image, and the filled image with red superpixel boundaries.

## Build

Requirements: Windows, Visual Studio 2022 and OpenCV 4.9.0. The project uses the course's `OpenCVApplication-VS2022_OCV490_basic` template layout, where the project file expects OpenCV in `OpenCV/Include` and `OpenCV/Lib` next to it. Those folders are not included here because of their size.

1. Copy the OpenCV `Include`, `Lib` and `dll` folders into this directory as `OpenCV/`, or change the include and library paths in the project properties to point to your own OpenCV 4.9.0 installation.
2. Open `OpenCVApplication.sln`, build, and run from the project directory so that `Data_Set/` is found.

To use another image, change the path in `main()` in `OpenCVApplication.cpp`.
