import 'dart:math';
import 'dart:io';

void main(List<String> args) { 

  var n = args.length > 0 ? int.parse(args[0]) : 48 * 50;

  var inc = pi / (n-1);
  var xs = new List<double>.generate(n, (int i) => cos(i * inc + pi) * 0.5 + 0.5);

  for (var x in xs) { print("$x,"); }
}