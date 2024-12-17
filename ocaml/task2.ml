(**
 * Derek Hessinger
 * Prof. Ying Li
 * CS 333
 * 12/6/24
 * 
 * This file performs a pixel wise operation to a given ppg file, taking in
 * arguments for the file name and the number of threads to use
 * 
 *  ocamlopt -o task2 task2.ml
 * ./task2 <file name> <num of threads>
*)

open Printf
open Domain

(*create pixel type*)
type pixel = {
  mutable r : int;
  mutable g : int;
  mutable b : int;
}

(*create image type*)
type image = {
  width : int;
  height : int;
  max_color : int;
  pixels : pixel array;
}

(*num iterations*)
let iterations = 100

(* get current time in seconds *)
let get_time () = Sys.time()

(* process a single pixel *)
let process_pixel px =
  px.r <- if px.r > 128 then (200 + px.r) / 2 else (30 + px.r) / 2;
  px.g <- if px.g > 128 then (200 + px.g) / 2 else (30 + px.g) / 2;
  px.b <- if px.b > 128 then (200 + px.b) / 2 else (30 + px.b) / 2

(* Process a chunk of the image *)
let process_chunk start_idx end_idx pixels =
  for _iter = 1 to iterations do
    for i = start_idx to end_idx - 1 do
      process_pixel pixels.(i)
    done
  done

(* read in ppm image *)
let read_ppm filename =
  
  let ic = open_in_bin filename in
  let line = input_line ic in
  if line <> "P6" then failwith "Not a PPM file";
  let rec read_header () =
    let line = input_line ic in
    if line.[0] = '#' then read_header ()
    else line
  in
  let dims = read_header () in
  let [width; height] = List.map int_of_string (String.split_on_char ' ' dims) in
  let max_color = int_of_string (input_line ic) in
  let pixels = Array.init (width * height) (fun _ ->
    let r = input_byte ic in
    let g = input_byte ic in
    let b = input_byte ic in
    {r; g; b}
  ) in
  close_in ic;
  {width; height; max_color; pixels}

(* Write PPM image - this would need to interface with your ppmIO *)
let write_ppm img filename =
  let oc = open_out_bin filename in
  fprintf oc "P6\n%d %d\n%d\n" img.width img.height img.max_color;
  Array.iter (fun px ->
    output_byte oc px.r;
    output_byte oc px.g;
    output_byte oc px.b;
  ) img.pixels;
  close_out oc

(* Main parallel processing function *)
let process_image_parallel num_threads img =
  let total_pixels = Array.length img.pixels in
  let chunk_size = total_pixels / num_threads in
  
  let start_time = get_time () in
  
  (* Create and run domains *)
  let domains = Array.init num_threads (fun i ->
    let start_idx = i * chunk_size in
    let end_idx = if i = num_threads - 1 then total_pixels 
                  else (i + 1) * chunk_size in
    Domain.spawn (fun () -> 
      process_chunk start_idx end_idx img.pixels
    )
  ) in
  
  (* Wait for all domains to complete *)
  Array.iter Domain.join domains;
  
  let end_time = get_time () in
  let total_time = end_time -. start_time in
  
  (* Print timing results *)
  printf "Image processing completed:\n";
  printf "Number of threads: %d\n" num_threads;
  printf "Number of iterations: %d\n" iterations;
  printf "Total time: %.3f seconds\n" total_time;
  printf "Time per iteration: %.3f seconds\n" (total_time /. float_of_int iterations);
  printf "Image dimensions: %d x %d (%d pixels)\n" 
    img.width img.height total_pixels

let () =
  if Array.length Sys.argv < 3 then begin
    printf "Usage: %s <image filename> <number of threads (1,2,4)>\n" Sys.argv.(0);
    exit 1
  end;
  
  let num_threads = int_of_string Sys.argv.(2) in
  if not (List.mem num_threads [1; 2; 4]) then begin
    printf "Number of threads must be 1, 2, or 4\n";
    exit 1
  end;
  
  try
    let img = read_ppm Sys.argv.(1) in
    process_image_parallel num_threads img;
    write_ppm img "updated_img.ppm"
  with 
  | Sys_error msg -> 
      printf "Error reading/writing file: %s\n" msg;
      exit 1
  | Failure msg -> 
      printf "Error processing image: %s\n" msg;
      exit 1