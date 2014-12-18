#!/usr/bin/env ruby

require 'prime'

#
#
#
def pulse(timestep=0,*tmpargs)
  args = Hash[*tmpargs]
  frequency  = ( args.has_key?("frequency")  ? args["frequency"].to_f  : 10.0   )
  gain       = ( args.has_key?("gain")       ? args["gain"].to_f       : 1.0    )
  tolerance  = ( args.has_key?("tolerance")  ? args["tolerance"].to_f  : 0.001  )
  phase      = ( args.has_key?("phase")      ? args["phase"].to_f      : 0.0    )
 
  tmpval = ((frequency* timestep) / 0.5)

  if timestep == 0 
    gain*1
  elsif ((tmpval.round - tmpval).abs < tolerance && (tmpval.round % 2 == 0) ) 
    gain*1
  elsif ( Math.sin(2*Math::PI*frequency*timestep+phase) < 0 ) || ((tmpval.round - tmpval).abs < tolerance && (tmpval.round % 2 == 1) ) 
    0
  else
    gain*1
  end
end

def sin(timestep=0,*tmpargs)
  args = Hash[*tmpargs]
  frequency  = ( args.has_key?("frequency")  ? args["frequency"].to_f  : 10.0   )
  gain       = ( args.has_key?("gain")       ? args["gain"].to_f       : 1.0    )
  tolerance  = ( args.has_key?("tolerance")  ? args["tolerance"].to_f  : 0.001  )
  phase      = ( args.has_key?("phase")      ? args["phase"].to_f      : 0.0    )
  Math.sin(2*Math::PI*frequency*timestep+phase )
end


#
# Generates a simple table 
#
def generate_table(functions=[:pulse,:pulse,:pulse],frequencies=[40,44,48], points_per_period=2 )
  timer,number_points = calculate_sample_rates( frequencies, points_per_period )
  (0..number_points-1 ).each { |ts|
    timestep = ts *  ( 1e-6 * timer.to_f)
    functions.each_with_index { |fn,i| 
      print sprintf("%.1f",(method( fn ).call( ts=timestep, "frequency" => frequencies[i] ))) + ","
    }
    print "EOD\n"
  }
  print "LOOP\n"
end

#
#
#
def calculate_sample_rates(frequencies=[40,44,48], points_per_period=2 , clock_resolution = 1_000_000 ) 
  if frequencies.find { |i| i.class == Rational } 
    # Need to up convert each rational into an integer
    tmpfreqs = frequencies.map { |i| (i * i.denominator).to_i }
  else
    tmpfreqs = frequencies
  end
  
  tmp_num = tmpfreqs.inject(:lcm).to_r

  repeat_point = 1 / (tmpfreqs.inject(:gcd).to_r)

  common_num = Hash[*tmpfreqs.map { |freq| (repeat_point / ( 1 / freq.to_r )).to_i  }.map { |i| i.prime_division.flatten }.sort.flatten].map{|k,v|k**v}.inject(1) {|p,i| p*=i }

  counter_number = points_per_period * common_num

  counter_increment = ( (repeat_point / (common_num * points_per_period )) * clock_resolution ).to_i

  min_size = ( tmpfreqs.size + 1 ) * counter_number + 1

  [counter_increment, counter_number, min_size ]

end

