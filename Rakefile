# This is only required for developers and has some scripts which test data
# integrity and such


namespace :integrity do
  OBJECT_TYPES = %w|object objicon misc explosion edge floor wall monster statue figurine cursor|

  namespace :tiles do

    desc "Redundancy check of all tile graphics"
    task :redundancy do
      files = Dir[File.join( 'vulture', 'gamedata', 'tiles', '*' )].map{|n|'tiles/'+n.split('/')[-1]}
      object_match = /^\s*(?:#{OBJECT_TYPES.join('|')})\.(\S+)\s*=(?:>\s*(?:#{OBJECT_TYPES.join('|')})\.(\S+)|\s*"([^"]+)"\s+(-?\d+)\s+(-?\d+))\s*$/
      File.readlines( File.join( 'vulture', 'gamedata', 'config', 'vulture_tiles.conf' ) ).each do |line|
        match = line.match( object_match )
        if match
          if match[3]
            #p match[3]
            files.delete( match[3] )
          end
        end
      end
      if files.size > 0
        puts "The following tiles aren't ever used:"
        files.each do |tile|
          puts "\t#{tile}"
        end
      end
    end

    namespace :existance do

      OBJECT_TYPES.each do |object_type|
        desc "Integrity check of #{object_type} data inside config/vulture_tiles.conf"
        task object_type.to_sym do
          object_match = /^\s*#{object_type}\.(\S+)\s*=(?:>\s*#{object_type}\.(\S+)|\s*"([^"]+)"\s+(-?\d+)\s+(-?\d+))\s*$/
          objects = {}
          File.readlines( File.join( 'vulture', 'gamedata', 'config', 'vulture_tiles.conf' ) ).each do |line|
            match = line.match( object_match )
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
              puts "#{object_type}:#{key} references unknown reference: #{objects[key][ :reference ]}" unless objects.has_key?( objects[key][ :reference ] )
            else
              puts "#{object_type}:#{key} references missing file: #{objects[key][ :graphic ]}" unless File.exists?( File.join( 'vulture', 'gamedata', objects[key][ :graphic ] ) )
            end
          end
        end
      end
    end
  end

  desc "Run all integrity checks"
  task :check => [OBJECT_TYPES.map{|n|"tiles:existance:#{n}"},'tiles:redundancy'].flatten

end

task :default => 'integrity:check'
