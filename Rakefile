# This is only required for developers and has some scripts which test data
# integrity and such

namespace :integrity do
  namespace :tiles do
    desc "Integrity check of object data inside config/vulture_tiles.conf"
    task :object do
      OBJECT_MATCH = /^\s*object\.(\S+)\s*=(?:>\s*object\.(\S+)|\s*"([^"]+)"\s+(-?\d+)\s+(-?\d+))\s*$/
      objects = {}
      File.readlines( File.join( 'vulture', 'gamedata', 'config', 'vulture_tiles.conf' ) ).each do |line|
        match = line.match( OBJECT_MATCH )
        if match
          objects[match[1]] = {}
          if match[2]
            objects[match[1]][ :reference ] = match[2]
          else
            objects[match[1]][ :graphic ] = match[3]
            objects[match[1]][ :offset ] = [ match[4].to_i, match[5].to_i ]
          end
        end
      end
      objects.keys.each do |key|
        if objects[key].has_key?( :reference )
          puts "#{key} references unknown reference: #{objects[key][ :reference ]}" unless objects.has_key?( objects[key][ :reference ] )
        else
          puts "#{key} references missing file: #{objects[key][ :graphic ]}" unless File.exists?( File.join( 'vulture', 'gamedata', objects[key][ :graphic ] ) )
        end
      end
    end
  end
  desc "Run all integrity checks"
  task :check => %w|tiles:object|
end
