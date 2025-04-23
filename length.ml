let rec len = function
 [] -> 0
| h::t -> 1 + len t;;

len [1;2;3;100];;
