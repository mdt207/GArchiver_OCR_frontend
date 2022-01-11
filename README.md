# GArchiver_OCR_frontend
An utility for GArchiver application to enter data from scanned images.
This is a front-end for tesseract-ocr for obtaining data and entering it to the GArchiver application's informations rows.
An application depends on from the following components: GTK+-3, Cairo - 2D graphics library, libvips - image processing system, Ruby (programming language), Tesseract - an optical character recognition engine.

Using GArchiver_OCR_frontend on Windows 10.
Needed applications versions that are used for deployment:
 1. rubyinstaller-devkit-3.0.3-1-x64.exe
 2. tesseract-ocr-w64-setup-v5.0.0.20211201.exe
 3. vips-dev-w64-all-8.12.1.zip
 
Make sure that the "MSYS2 development toolchain" check box is selected during the Ruby installation process. It's necessity for adding ruby-vips into the gem collection.
In config.txt file enter absolute path to Tesseract-OCR and Ruby executables.

e.g lines in config.txt (without the line numbers):
 
  1. C:\Ruby30-x64\bin\ruby.exe
  2. C:\msys\home\mdt\prjs\ruby\subtitle_extractor\convert_to_text.rb

