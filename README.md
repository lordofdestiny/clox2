# clox2

## About this project
This project is an implementation of a programing language, inspiration for which was taken from Bobert Nystrom's book ["Crafting Interpreters"](https://craftinginterpreters.com/).
The language shares the basic sytax with the Lox programming language Bob invented for his book, but is extended in 
syntax in features I have deemed useful or interesting. Currenlty, the language doesn't have a name, even though the repository
references it's inspiration. The language is still a work in progress and is in a process of gaining it's own identiy. 

## About the implementation
As it is the case with Lox, this language is implemented in C, as a bytecode interpreted language. There are no plans for
implementing a JIT at this time, but that might change in the future. Many features are yet to be implemented, and no feature
is consider stable. Lox can be used for some light scripting and calculations, but since it is lacking a standard library,
and elemental features like arrays, it is not a production ready language at this point.
