#!/usr/bin/ruby

require 'pathname'
require 'open3'
require 'vips'

#SRC_DIR = '/path/to/src/dir/'.freeze
#TMP_DIR = '/path/to/tmp/dir/'.freeze

#SRC_DIR = '../cache/'.freeze
#TMP_DIR = '../cache/tmp/'.freeze


@WORK_DIR = __dir__#+'../cache/tmp/'.freeze
@TMP_DIR  = @WORK_DIR

#p @WORK_DIR

Dir.chdir(@WORK_DIR)

@amount_of_files = 0
@sequience_of_files = File.readlines("list_roi.txt")


#form the SRC_DIR path based on source path taken from list_roi.txt
SRC_DIR = File.dirname(@sequience_of_files[0].to_s)



#Dir.chdir('../cache')
Dir.chdir(@WORK_DIR.to_s + '/cache')
@TMP_DIR = Dir.getwd
@TMP_DIR += '/tmp'
Dir.mkdir @TMP_DIR unless Dir.exists? ('tmp')

class TextReader
  #def initialize(input_path, output_path)
  def initialize (tesseract_psm)
    #@input_path  = input_path
    #@output_path = output_path
    #@out_fname   = File.dirname(@output_path)
    @iter_num = 1
    @ocr_lang = "eng+uzb_cyrl"
    @tesseract_psm = tesseract_psm
  end

  def read(input_path, output_path)

    @input_path  = input_path
    @output_path = output_path
    @out_fname   = File.dirname(@output_path)
    # prepare the image
    #
    # Read the image as grayscale

    if File.file?(@input_path)
    #p "#{@input_path}"
        img  = Vips::Image.new_from_file(@input_path.to_s, access: :sequential).colourspace('b-w')
    #
    # convert the image to binary; into black and white
        img_bw = img > 237  # that's the threshold
        img_bw.write_to_file(@output_path)
	else
	 p "input png file not exists."
	 return -1
	end


    #p @out_fname
    if File.file?(@input_path)
      #@out_fname = File.join(@out_fname, @input_path.basename.to_s.gsub(@input_path.extname, ".txt"))
      @file_name = File.join(@out_fname, @input_path.basename.to_s.gsub(@input_path.extname, ''))
      p "file name: #{@file_name}"
      #p @input_path.basename
    end #of if File.file?(@input_path)
    # read the text and return it

    #p "tesseract #{@output_path} #{@file_name} -l #{@ocr_lang} --psm #{@tesseract_psm}"

    #text, _,  _ =
    text, err, s =
      #Open3.capture3("tesseract #{@output_path} stdout -l eng+uzb_cyrl --oem 0 --psm 3")
      #Open3.capture3("tesseract #{@output_path} stdout -l eng+uzb_cyrl --psm 3")
      #Open3.capture3("tesseract #{@output_path} #{@file_name} -l eng+uzb_cyrl --psm 13")

      Open3.capture3("tesseract #{@output_path} #{@file_name} -l #{@ocr_lang} --psm #{@tesseract_psm}")

      #ocr_file_name = File.join(@out_fname.to_s, @file_name.to_s)
      ocr_file_name = @file_name.to_s + ".txt"
      if File.exists?(ocr_file_name.to_s) #tesseract produced text file's full path and name

        ocr_files_list = File.new("list_ocr_files.txt", "w+") unless File.exists?("list_ocr_files.txt")
		ocr_files_list = File.open("list_ocr_files.txt", "a") if File.exists?("list_ocr_files.txt")
        ocr_files_list.puts(ocr_file_name)
        p ocr_file_name
        ocr_files_list.close

	ocr_file = File.new("ocr_file.txt", "w+") unless File.exists?("list_ocr_files.txt")
	ocr_file = File.open("ocr_file.txt", "a") if File.exists?("list_ocr_files.txt")
	all_sentence = ""
        File.open(ocr_file_name) do |f| #reading from tesseract produced text file
             f.each do
		|str|
		str = str.gsub(/[\n\t]$/,' ')
		str = str.sub(/[\f]$/,'')
		#str = str.strip
		all_sentence << str
		end
	
		all_sentence = all_sentence.sub(/\s$/,"") #remove trailing space
		p "#{all_sentence}"
		ocr_file.write("#"+ @iter_num.to_s + " " + all_sentence) #form necessary sentence (data string with tag)
		#ocr_file.write("#"+ @iter_num.to_s + " " + all_sentence)
		#str = f.gets.strip
         #str = str.gsub(/\n$/,'')
		#ocr_file.write("#"+ @iter_num.to_s + " " + str)
		#p str
      end
	ocr_file.puts()
	ocr_file.close

		if (s.success?)
			p @iter_num.to_s
			@iter_num += 1
		end
    end
     #text.strip
  end#Open3.capture3

end



def check_file_name(file_name)
  while(@amount_of_files < @sequience_of_files.length)
    #p @amount_of_files
    fname = @sequience_of_files[@amount_of_files].to_s

    fname = fname.gsub(/^\s$/,'')
    fname = fname.gsub(/\n$/,'')
    tmp   = File.basename(fname)
    #p "sole name:#{tmp}"
    #print file_name
    #print " "
    #print fname+"\n"
    @amount_of_files += 1
    #return true if (fname.to_s == file_name.to_s)
    #if (fname.to_s == file_name.to_s)
    if (tmp.to_s == file_name.to_s)
	@amount_of_files = 0
	return true
    end

  end
end


newly_renamed = 0
not_renamed   = 0

psm = "3"
psm = ARGV[0] if ARGV[0] != nil
p psm

#Dir.mkdir TMP_DIR unless File.exists?(TMP_DIR)
Dir.chdir(@WORK_DIR)
ocr_files_list = File.new("list_ocr_files.txt", "w+") if File.exists?("list_ocr_files.txt")
ocr_files_list.close


new_name = TextReader.new(psm.to_s)

Pathname.new(SRC_DIR).children.each do
#Dir.entries(SRC_DIR).select {
|f|   if (File.file?(f) && check_file_name(f.basename))#f.basename.to_s == "test7.png")
	#p f.basename
	#new_name = TextReader.new(f.realpath,"#{TMP_DIR}/#{f.basename}").read
	new_name.read(f.realpath,"#{@TMP_DIR}/#{f.basename}")

     else @amount_of_files = 0
    #@amount_of_files = 0 if(@amount_of_files == @sequience_of_files.length)
 end
#|f|  new_name = TextReader.new(f.realpath,"#{TMP_DIR}/#{f.basename}").read if (File.file?(f) && check_file_name(f.basename))
    #File.file? new_name = TextReader.new(f.realpath,
     #  "#{TMP_DIR}/#{f.basename}")
    #.read
    #.downcase
    #.gsub(/[[:punct:]]/, ' ')
    #.split
    #.join('-')
  #File.file?(f) ? p f.basename : p "not file";
  # the 252 byte limit is for the ext4 file system limit of 255 bytes per filename
  # for some reason, the actual limit in Ruby's Pathname#rename is 252

  #new_name = if !new_name.empty? && new_name.bytes.size < 252
#               newly_renamed += 1
#               new_name
#             else
#               not_renamed += 1
#               f.basename
#             end

  #f.rename("#{SRC_DIR}/#{new_name}#{f.extname}")
end
#}

puts "*" * 50
puts "#{newly_renamed} files newly renamed vs. #{not_renamed} still have the same old name."
