package graph

// import (
//   "testing"
//   "rstk/internal/parser"
// )

// Global test topology, instead of defining it in a file, it is directly 
// setted up from AS relation list. All the tests functions references
// this topology
// 
// Topology structure is taken from the ASPA IETF draft, and the NOMS-24 paper
// 
//
// 	          ------ 1 -----
//           /       |      \
//          2  ----- 3       4
//         /  \    /   \   /   \
//        /    \  /     \ /     \
//       5 ----- 6 ----- 7 ----- 8
//      / \     / \     / \     / \
//     /   \   /   \   /   \   /   \
//    9    10 11   12 13   14 15   16
//   /                               \
//  /                                 \
// 17                                  18
//

// asRelsList := []parser.ASRel{
//   {AS1: 1, AS2: 2, Relation}
// }
//
// func TestInit
