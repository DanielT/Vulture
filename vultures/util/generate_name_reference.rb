%w{nethack slashem}.each do |reference|
    File.open("references/#{reference}") do |file|
        lines = file.readlines
        referencefile = File.new( "../vultures_tilename_reference_#{reference}.h", "w")
        referencefile << "char *tilename_reference[#{lines.length}] = {\n"
        lines.each_with_index {|line,index| referencefile << "\t\t\"#{line.strip}\"#{index==lines.length-1?'':','}\n" }
        referencefile << "\t};\n"
        referencefile.close
    end
end
