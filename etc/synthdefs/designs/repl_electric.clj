(ns sonic-pi.synths.repl-electric
  (:use [overtone.live])
  (:require [sonic-pi.synths.core :as core]))

(without-namespace-in-synthdef
 (defsynth sonic-pi-leadsaw [amp 1 noise-level 0.05 attack 0.4 release 0.9 saw-cutoff 300 noise-cutoff 100 wave 1
                  note 52
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
                  attack 1e-8
                  decay 1e-6
                  sustain 0.75
                  release 0.125
                  attack_level 1
                  sustain_level 1
                  env_curve 2
                  out_bus 0
                  cutoff 100
                  cutoff_slide 0
                  cutoff_slide_shape 5
                  cutoff_slide_curve 0]
   (let [note      (varlag note note_slide note_slide_curve note_slide_shape)
         amp       (varlag amp amp_slide amp_slide_curve amp_slide_shape)
         pan       (varlag pan pan_slide pan_slide_curve pan_slide_shape)
         amp-fudge 1

         freq (midicps note)
         cutoff-freq (midicps cutoff)
         cutoff      (varlag cutoff cutoff_slide cutoff_slide_curve cutoff_slide_shape)

         nvoices1 5
         detune2 8
         detune1 12
         env (env-gen:kr (env-adsr-ng attack decay sustain release attack_level sustain_level) :action FREE)

         osc2 [(let [m (pow 2 (rand (/ detune2 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m freq)) 0.02 (* (rand) (reciprocal freq) lfo))]
                 w)
               (let [m (pow 2 (rand (/ detune2 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m freq)) 0.02 (* (rand)  (reciprocal freq) lfo))]
                 w)
               (let [m (pow 2 (rand (/ detune2 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m freq)) 0.02 (* (rand)  (reciprocal freq) lfo))]
                 w)
               (let [m (pow 2 (rand (/ detune2 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m freq)) 0.02 (* (rand) (reciprocal freq) lfo))]
                 w)
               (let [m (pow 2 (rand (/ detune2 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m freq)) 0.02 (* (rand) (reciprocal freq) lfo))]
                 w)
               (let [m (pow 2 (rand (/ detune2 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m freq)) 0.02 (* (rand) (reciprocal freq) lfo))]
                 w)
               (let [m (pow 2 (rand (/ detune2 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m freq)) 0.02 (* (rand) (reciprocal freq) lfo))]
                 w)]

         osc1 [(let [m (pow 2 (rand (/ detune1 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m (/ freq 2.0))) 0.02 (* (rand) (reciprocal freq) lfo))]
                 w)
               (let [m (pow 2 (rand (/ detune1 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m  (/ freq 2.0))) 0.02 (* (rand) (reciprocal freq) lfo))]
                 w)
               (let [m (pow 2 (rand (/ detune1 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m  (/ freq 2.0))) 0.02 (* (rand) (reciprocal freq) lfo))]
                 w)
               (let [m (pow 2 (rand (/ detune1 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m  (/ freq 2.0))) 0.02 (* (rand) (reciprocal freq) lfo))]
                 w)
               (let [m (pow 2 (rand (/ detune1 1200)))                                          ;;
                     lfo (range-lin (sin-osc:ar (rand 3)) 0.0 1.0)                                      ;;
                     w  (delay-c:ar (lf-saw:ar (* m  (/ freq 2.0))) 0.02 (* (rand) (reciprocal freq) lfo))]
                 w)]

         src (+ (b-low-pass osc1 800 0.5) (/ osc2 4))
         src (splay:ar src)
         src (lpf src cutoff-freq)
         src (* amp src env)]
     (out out_bus (pan2 (* amp-fudge env src) pan amp)))))

(comment
  (core/save-synthdef sonic-pi-leadsaw)
  (sonic-pi-leadsaw :wave 0))


(without-namespace-in-synthdef
 (defsynth sonic-pi-gpa [amp 1 noise-level 0.05 attack 0.4 release 0.9 saw-cutoff 300 noise-cutoff 100 wave 1
                                        note 52
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
                                        out_bus 0
                                        cutoff 100                                                                cutoff_slide 0
                                        cutoff_slide_shape 5
                                        cutoff_slide_curve 0]
   (let [note      (varlag note note_slide note_slide_curve note_slide_shape)
         amp       (varlag amp amp_slide amp_slide_curve amp_slide_shape)
         pan       (varlag pan pan_slide pan_slide_curve pan_slide_shape)
         amp-fudge 1
         freq (midicps note)
         cutoff-freq (midicps cutoff)
         cutoff    (varlag cutoff cutoff_slide cutoff_slide_curve cutoff_slide_shape)


          noize (* noise-level (pink-noise))
          wave (select:ar wave [(mix [(lpf (lf-saw freq) saw-cutoff) (lf-tri freq)])
                                (lpf (saw freq) saw-cutoff)
                                (lpf (pulse freq) saw-cutoff)
                                (mix [(lpf (saw freq) saw-cutoff) (pulse freq)])
                                (sum [(lpf (saw (/ freq 2)) saw-cutoff) (lf-tri freq)])
                                (mix [(pitch-shift (lpf (sin-osc freq) saw-cutoff) 0.4 1 0 0.01)])])
          src (mix [wave
                    (lpf noize noise-cutoff)])
          src (g-verb src 200 1 0.2)
          src (lpf src cutoff-freq)
          env (env-gen:kr (env-adsr-ng attack decay sustain release attack_level sustain_level) :action FREE)]
     (out out_bus (pan2 (* amp-fudge env src) pan amp)))))

(comment
  (core/save-synthdef sonic-pi-gpa)
  (sonic-pi-gpa :wave 0)
  )

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
                             out_bus 0
                             cutoff 100
                             cutoff_slide 0
                             cutoff_slide_shape 5
                             cutoff_slide_curve 0

                             blip_rate 0.5
                             decay_time 0.9
                             decay_coef 0.4
                             room 200
                             reverb 8
                             ]
   (let [note      (varlag note note_slide note_slide_curve note_slide_shape)
         amp       (varlag amp amp_slide amp_slide_curve amp_slide_shape)
         pan       (varlag pan pan_slide pan_slide_curve pan_slide_shape)
         cutoff    (varlag cutoff cutoff_slide cutoff_slide_curve cutoff_slide_shape)
         cutoff-freq (midicps cutoff)
         freq (midicps note)
         amp-fudge 1
         src (sum [(sin-osc freq)
                   (saw freq)
                   (blip freq (* blip_rate (sin-osc:kr blip_rate)))])
         dly  (/ 1 freq)
         src (pluck src 1 dly dly decay_time (min  decay_coef 0.99))
         src (rlpf src 1000)
         src (g-verb src (max room,1) reverb)
         src (lpf src cutoff-freq)
         env (env-gen:kr (env-adsr-ng attack decay sustain release attack_level sustain_level) :action FREE)]
     (out out_bus (pan2 (* amp-fudge env src) pan amp)))))

(comment
  (core/save-synthdef sonic-pi-plucked)

  (sonic-pi-plucked :amp 2 :note 52 :blip_rate 1.0 :decay_time 1.9  :decay_coef 1.1 :room 1 :reverb 0)
  )

(without-namespace-in-synthdef
 (defsynth sonic-pi-twang [overtones 1.5
                           decay 3
                           sustain 1
                           note 52
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
              amp-fudge 1.0

              src  (blip freq (+ overtones (* 0.5 (sin-osc:kr 20))))
              o1 (comb-l:ar (/ (tanh src) 8) 1 1 8)
              o2 (comb-l:ar (/ (tanh src) 8) 1 1 8)
              o1 (free-verb o1 :mix (+ 0.5 (* 0.5 (sin-osc:kr 0.5))) :room 5)
              o2 (free-verb o2 :mix (+ 0.5 (* 0.2 (sin-osc:kr 1.0))) :room 5)
              src [o1 o2]
              env (env-gen:kr (env-adsr-ng attack decay sustain release attack_level sustain_level) :action FREE)
              ]
          (out out_bus (pan2 (* (+ 3 amp-fudge) env src) pan amp)))))


(comment
  (kill sonic-pi-twang)
  (core/save-synthdef sonic-pi-twang)
  (sonic-pi-twang :release 24.0 :attack 0.01)
  )

(without-namespace-in-synthdef
 (defsynth sonic-pi-string [overtones 1.5
                            decay 3
                            sustain 1
                            note 52
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
                            out_bus 0
                            dur 1
                            amp 0.8 decay 30 coef 0.3 gate 1
                            release 0.2
                            attack 0.03
                            damp 0.2
                            coef-buf 0
                            beat-bus 0 beat-trg-bus 0
                            notes-buf 0 dur-buf 0
                            mix-rate 0.5]
   (let [note      (varlag note note_slide note_slide_curve note_slide_shape)
         amp       (varlag amp amp_slide amp_slide_curve amp_slide_shape)
         pan       (varlag pan pan_slide pan_slide_curve pan_slide_shape)
         freq (midicps note)
         amp-fudge 1.0

         freq   (midicps note)
         noize  (* (lf-tri freq (sin-osc:kr 0.5)))
         dly    (/ 1.0 freq)
         plk    (pluck noize 1.0 dly dly decay coef)
         dist   (distort plk)
         filt   (rlpf dist (* 12 freq) 0.6)
         clp    (clip2 filt 0.8)
         clp (mix [clp
                   (* 1.01 (sin-osc freq (* 2 Math/PI)))
                   (rlpf (saw freq) 1200)])
         clp (comb-n clp 0.9)
         ;;reverb (g-verb clp 400 2.0 0.0 0.1 0.3 0.2 0.5 0.1 400)
         src (g-verb clp 250 20 0)
         env (env-gen:kr (env-adsr-ng attack decay sustain release attack_level sustain_level) :action FREE)]
     (out out_bus (pan2 (* amp-fudge env src) pan amp)))))

(comment
  (core/save-synthdef sonic-pi-string)
  (sonic-pi-string :dur 0.1 :release 0.1 :attack 0.1 :decay 0.2 :note 50)
  (kill sonic-pi-string)
  )

(without-namespace-in-synthdef
 (defsynth sonic-pi-wobbling
   [amp 0.8 t 4
    mix-rate 0.8 room-rate 0.5

    amt 0.3
    notes-buf 0 dur-buf 0
    max-delay 0.4
    delay 0.3
    decay 0.5

    overtones 1.5
    decay 3
    sustain 1
    note 52
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
    out_bus 0
    dur 1
    amp 0.8 decay 30 coef 0.3 gate 1
    release 0.2
    attack 0.03
    damp 0.2
    coef-buf 0
    beat-bus 0 beat-trg-bus 0
    notes-buf 0 dur-buf 0
    mix-rate 0.5
    ]
   (let [note      (varlag note note_slide note_slide_curve note_slide_shape)
         amp       (varlag amp amp_slide amp_slide_curve amp_slide_shape)
         pan       (varlag pan pan_slide pan_slide_curve pan_slide_shape)
         freq (midicps note)
         amp-fudge 1.0
         freq (midicps note)

         f-env      (env-gen (perc t t) 1.0 1 0 1)
         src        (saw [freq (* freq 1.01)])
         signal     (rlpf (* 0.3 src)
                          (+ (* 0.6 freq) (* f-env 2 freq)) 0.2)
         k          (/ (* 2 amt) (- 1 amt))
         distort    (/ (* (+ 2 k) signal) (+ 2 (* k (abs signal))))
         gate       (pulse (* 2 (+ 1 (sin-osc:kr 0.05))))
         compressor (compander distort gate 0.01 1 0.5 0.01 0.01)
         dampener   (+ 1 (* 0.5 (sin-osc:kr 0.5)))
         reverb     (free-verb compressor mix-rate room-rate dampener)
         echo       (comb-n reverb max-delay delay decay)
         env (env-gen:kr (env-adsr-ng attack decay sustain release attack_level sustain_level) :action FREE)]
     (out out_bus (pan2 (* amp-fudge env echo) pan amp)))))

(comment
  (def w (sonic-pi-wobbling))
    (ctl w :note 50)

    (core/save-synthdef sonic-pi-wobbling)

  (kill sonic-pi-wobbling)
  )
