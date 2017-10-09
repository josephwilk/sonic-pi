#--
# This file is part of Sonic Pi: http://sonic-pi.net
# Full project source: https://github.com/samaaron/sonic-pi
# License: https://github.com/samaaron/sonic-pi/blob/master/LICENSE.md
#
# Copyright 2013, 2014, 2015, 2016 by Sam Aaron (http://sam.aaron.name).
# All rights reserved.
#
# Permission is granted for use, copying, modification, and
# distribution of modified versions of this work as long as this
# notice is included.
#++

require_relative "lang/support/docsystem"

module SonicPi
  module PreParser

    class PreParseError < StandardError ; end

    def self.preparse(rb, vec_fns)
      ings = rb.scan(/(\(ing\s+|\(nit\s+|\(ine\s+)([^\)]+)(\))/)
      #ings = rb.scan(/(\(ing\s+)([^\)]+)(\))/)
      #puts ings.inspect
      ings.each do |ing|
        #puts ing.inspect
        target = ing[1].split(/\n+/)
        target = target.map {|t|
          rs = t.split(/#[^{]{1}/)
          rs[0]
        }.flatten.join("\n")
        target = target.split(/\s+/).join(",")
        new_ings = ing[0] + target + ing[2]
        old_ings = ing[0] + ing[1] + ing[2]
        #puts "newing: #{new_ings}"
        #puts "olding: #{old_ings}"
        rb.gsub!(old_ings, new_ings)
      end


      vec_fns.each do |fn|
        rb = String.new(rb)
        fn = fn[:name].to_s
        rb.gsub!(/\((\s*)#{fn}([,[:space:]]+)/) {|s| ' ' + $1 + fn + '(' + (' ' * ($2.size - 1))}

        rb.gsub!(/:([a-zA-Z0-9\!\?=_]+(:[a-zA-Z0-9\!\?=_]+[a-zA-Z0-9\!\?=_])+)/){|s| "::SonicPi::SPSym.new('#{$1.split(':').join(' : ')}')"}

        if rb.match(/(?!\B)\W?#{fn}\s*=[\s\w]/)
          raise PreParseError, "You may not use the built-in fn names as variable names.\n You attempted to use: #{fn}"
        end
      end
      #Lets invent our own Ruby Syntax using REGEXP. What could go wrong?
      rb.gsub!(/(\s\d+\.)(\z|\n|\s)/, "\\10\\2")          #  2.   => 2.0
      rb.gsub!(/([^\d\.]{1})(\.\d+)(\z|\n|\s|\)|)/, "\\10\\2\\3")   # .3   => 0.3
      #rb.gsub!(/\(dring ([^\)]+)\)/, "(ring %W{\\1})")
      rb
    end
  end
end
