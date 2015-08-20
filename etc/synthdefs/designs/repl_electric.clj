(ns sonic-pi.synths.repl-electric
  (:use [overtone.live])
  (:require [sonic-pi.synths.core :as core]))

(without-namespace-in-synthdef
 (defsynth sonic-pi-plucked [note 52
                             note_slide 0
                             note_slide_shape 5
                             note_slide_curve 0
                             amp 1
                             amp_slide 0
                             amp_slide_shape 5
                             amp_slide_curve 0
                             pan 0
                             pan_slide 0
                             pan_slide_shape 5
                             pan_slide_curve 0
                             attack 0
                             decay 0
                             sustain 0
                             release 0.2
                             attack_level 1
                             sustain_level 1
                             env_curve 2
                             out_bus 0]
   (let [note      (varlag note note_slide note_slide_curve note_slide_shape)
         amp       (varlag amp amp_slide amp_slide_curve amp_slide_shape)
         pan       (varlag pan pan_slide pan_slide_curve pan_slide_shape)
         freq (midicps note)
         amp-fudge 1
         src (sum [(sin-osc freq)
                   (saw freq)
                   (blip freq (* 0.5 (sin-osc:kr 0.5)))])
         dly  (/ 1 freq)
         src (pluck src 1 dly dly 0.9 0.4)
         src (rlpf src 1000)
         src (g-verb src 200 8)
         env (env-gen:kr (env-adsr-ng attack decay sustain release attack_level sustain_level) :action FREE)]
     (out out_bus (pan2 (* amp-fudge env src) pan amp)))))

(comment
  (core/save-synthdef sonic-pi-plucked)
  (sonic-pi-plucked :amp 0.2 :note 40 :release 0.5 :attack 0.1)
  )
