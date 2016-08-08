;;--
;; This file is part of Sonic Pi: http://sonic-pi.net
;; Full project source: https://github.com/samaaron/sonic-pi
;; License: https://github.com/samaaron/sonic-pi/blob/master/LICENSE.md
;;
;; Copyright 2013, 2014, 2015 by Sam Aaron (http://sam.aaron.name).
;; All rights reserved.
;;
;; Permission is granted for use, copying, modification, and
;; distribution of modified versions of this work as long as this
;; notice is included.
;;++

;; This file uses Overtone to compile the synths from Clojure to
;; SuperCollider compatible binary files. Overtone is Sonic Pi's big
;; brother. See: http://overtone.github.io

(ns sonic-pi.synths.core
  (:use [overtone.live])

  (:require [clojure.string :as str]))

;; Utility functions (for creating and storing synthdefs)

(def path-to-synthdefs "/Users/josephwilk/Workspace/josephwilk/c++/sonic-pi/etc/synthdefs")

(defn save-synthdef [sdef]
  (let [compiled (str path-to-synthdefs "/compiled/" (last (str/split (-> sdef :sdef :name) #"/")) ".scsyndef")
        gv       (str path-to-synthdefs "/graphviz/" (last (str/split (-> sdef :sdef :name) #"/")) ".dot")
        dot     (graphviz sdef)]

    (spit gv dot)
    (overtone.sc.machinery.synthdef/synthdef-write (:sdef sdef) compiled)
    :done))


(defcgen buffered-coin-gate
  "Deterministic coingate using random buffer"
  [buf {:doc "pre-allocated buffer containing random values between 0 and 1"}
   seed {:default 0, :doc "Offset into pre-allocated buffer. Acts as the seed"}
   prob {:default 1, :doc "Determines the possibility that the trigger is passed through as a value between 0 and 1"}
   trig {:doc "Incoming trigger signal"} ]
  ""
  (:kr (let [phase (+ seed (pulse-count trig))
             v     (buf-rd:kr 1 buf phase 1)
             res   (< v prob)
             ]
         res)))

(defn shaped-adsr
  "Non gated adsr envelope with shape"
  ([attack
    decay
    sustain
    release
    attack_level
    decay_level
    sustain_level
    env_curve]
   (shaped-adsr attack decay sustain release attack_level decay_level sustain_level env_curve 0))
  ([attack
    decay
    sustain
    release
    attack_level
    decay_level
    sustain_level
    env_curve
    min]
  [min 4 -99 -99
   attack_level  attack  env_curve 0
   decay_level   decay   env_curve 0
   sustain_level sustain env_curve 0
   min           release env_curve 0] )

  ([attack
    decay
    sustain
    release
    init_level
    attack_level
    decay_level
    sustain_level
    release_level
    env_curve]
  [init_level 4 -99 -99
   attack_level  attack  env_curve 0
   decay_level   decay   env_curve 0
   sustain_level sustain env_curve 0
   release_level release env_curve 0] ))


(defmacro def-fx
  "Shorthand mechanism for defining FX synths. Allows just the specification of the FX logic that will be inserted within default logic for handling amp, mix, pre_mix, and bus reading and writing.

This macro expects a name and two lists.

The name should be the same as you would use with a standard defsynth.

The first list is a list of arg names and defaults - these should be the additional args for the FX. It is important that you don't use any of the default FX arg names defined within the macro body such as amp, mix_slide, in_bus etc.

The second list is a partial let form. This will be sandwiched within the FX defsynth let statement after logic for reading the dry signal and handling pre_mix and before the logic for writing the wet signal and handling mixing. There are 2 'magic' symbols that will be bound - dry-l and dry-r which represent the left and right dry signals. These should be used to feed signals into any FX ugen trees. The partial let form should then bind output signals to wet-l and wet-r which will then be used in the post FX logic."

  [fx-name args partial-let]

  `(defsynth ~fx-name
     [~@args
      ~'amp 1
      ~'amp_slide 0
      ~'amp_slide_shape 1
      ~'amp_slide_curve 0
      ~'mix 1
      ~'mix_slide 0
      ~'mix_slide_shape 1
      ~'mix_slide_curve 0
      ~'pre_mix 1
      ~'pre_mix_slide 0
      ~'pre_mix_slide_shape 1
      ~'pre_mix_slide_curve 0
      ~'pre_amp 1
      ~'pre_amp_slide 0
      ~'pre_amp_slide_shape 1
      ~'pre_amp_slide_curve 0
      ~'in_bus 0
      ~'out_bus 0

      ]
     (let [~'fx-arg-in_bus             ~'in_bus
           ~'fx-arg-out_bus            ~'out_bus

           ~'fx-arg-amp                ~'(varlag amp amp_slide amp_slide_curve amp_slide_shape)
           ~'fx-arg-mix                ~'(varlag mix mix_slide mix_slide_curve mix_slide_shape)
           ~'fx-arg-mix                ~'(lin-lin (clip fx-arg-mix 0 1) 0 1 -1 1)
           ~'fx-arg-pre_mix            ~'(varlag pre_mix pre_mix_slide pre_mix_slide_curve pre_mix_slide_shape)
           ~'fx-arg-pre_mix            ~'(lin-lin (clip fx-arg-pre_mix 0 1) 0 1 -1 1)
           ~'fx-arg-pre_amp            ~'(varlag pre_amp pre_amp_slide pre_amp_slide_curve pre_amp_slide_shape)
           ~'[fx-arg-in-l fx-arg-in-r] ~'(* fx-arg-pre_amp (in fx-arg-in_bus 2))
           ~'fx-arg-pre-mix-dry-m      ~'(- 2 fx-arg-pre_mix)
           ~'fx-arg-pre-dry-l          ~'(* fx-arg-pre-mix-dry-m fx-arg-in-l)
           ~'fx-arg-pre-dry-r          ~'(* fx-arg-pre-mix-dry-m fx-arg-in-r)
           ~'dry-l                     ~'(* fx-arg-pre_mix fx-arg-in-l)
           ~'dry-r                     ~'(* fx-arg-pre_mix fx-arg-in-r)

           ~@partial-let

           ~'post-fx-l                 ~'(x-fade2 fx-arg-pre-dry-l wet-l fx-arg-pre_mix)
           ~'post-fx-r                 ~'(x-fade2 fx-arg-pre-dry-r wet-r fx-arg-pre_mix)
           ~'fin-l                     ~'(x-fade2 fx-arg-in-l post-fx-l fx-arg-mix fx-arg-amp)
           ~'fin-r                     ~'(x-fade2 fx-arg-in-r post-fx-r fx-arg-mix fx-arg-amp)]
       ~'(out fx-arg-out_bus [fin-l fin-r]))))
