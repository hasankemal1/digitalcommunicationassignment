Digital Signal Processing and Filtering Application
===================================================

This project is a Qt6-based signal analysis and filtering application that allows users to process audio files with noise and extract desired sound components, such as birdsongs or owl hoots, from background noise like jet engines or car horns.

Features
--------

- Multiple Filter Options: Includes predefined filtering options for audio cleanup.
- Noise Reduction: Specialized buttons for filtering out jet noise and horn sounds.
- FFT and Spectrogram: Visualizes the frequency domain before and after filtering.
- IFFT Reconstruction: Reconstructs the cleaned signal in the time domain.
- WAV Export: Save filtered signals as "outputifft.wav".

Technologies Used
-----------------

- Qt 6.7.0 (Core, Gui, Widgets, Multimedia, PrintSupport)
- VkFFT for high-performance FFT/IFFT
- Vulkan SDK 1.4.313.0
- QCustomPlot for plotting signals
- C++17

Directory Structure
-------------------

project/
├── CMakeLists.txt
├── include/                 # Header files
├── src/                     # Source files
│   ├── ui/                  # Qt Designer .ui files
│   └── ...
├── thirdparty/
│   ├── qcustomplot/
│   └── VkFFT/
├── resources/               # WAV files, icons, etc.
└── build/                   # CMake build output

How to Build
------------

Requirements:
- CMake 3.20+
- Qt 6.7.0
- Vulkan SDK 1.4.313.0
- Visual Studio 2019 (MSVC 2019)

Building steps:

1. Clone the repository:
   git clone https://github.com/yourusername/digicomassignment.git

2. Create a build directory:
   cd digicomassignment
   mkdir build
   cd build

3. Generate and build:
   cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2019_64" -DCMAKE_BUILD_TYPE=Release
   cmake --build .

How to Use
----------

1. Run the application.
2. Click "Select File" and load a .wav audio file.
3. Choose a filter type or use one of the special filter buttons for owl_beep.wav or birds_jet_noise.wav.
4. Click "Filter" to apply the filter.
5. View the frequency spectrum on the graph.
6. Click "Reconstitute" to see the time-domain filtered signal.
7. Use "Save Filtered Results" to export the result as outputifft.wav.

License
-------

This project is released under the MIT License.

Contact
-------

Created by: [hasan kemal karaman]
Contact: eco162913@gmail.com
For Digital communucation coursework.
