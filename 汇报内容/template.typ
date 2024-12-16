#import "@preview/codly:1.0.0": *
#import "@preview/tablex:0.0.9": tablex, rowspanx, colspanx


#let name = "王俊亚"
#let stuNum = "22307049"

#let template(title, doc) = {
  show ref: it => {
    set text(fill: orange)
    it
  }
  set page(
    paper: "us-letter",
    header: align(right)[
      #set text(font: "Monotype Corsiva")
      Computer Graphics
    ],
    numbering: "1/1",
  )

  show: codly-init.with()

  codly(
    languages: (
      C: (
        name: "C++",
        icon: text(font: "tabler-icons", "\u{fa53}"),
        color: rgb("#CE412B")
      ),
      CPP: (
        name: "C++",
        icon: text(font: "tabler-icons", "\u{fa19}"),
        color: rgb("#CE412B")
      ),
      cpp: (
        name: "C++",
        icon: text(font: "tabler-icons", "\u{fa19}"),
        color: rgb("#CE412B")
      ),
      GLSL: (
        name: "GLSL",
        icon: text(font: "tabler-icons", "\u{fa40}"),
        color: rgb("#2bceab")
      ),
      Python: (
        name: "Python",
        icon: text(font: "tabler-icons", "\u{fa57}"),
        color: rgb("#442bce")
      ),
    )
  )

  set par(justify: true)
  set document(author: name, title: title, date: auto)
  set text(font: "STSong", lang: "zh", region: "cn")
  show heading: set block(above: 2em, below: 1em)
  set heading(numbering: "I.1.1")
  show heading: it => block[
    #set text(font: "STZhongsong")
    #counter(heading).display() #strong(it.body)
  ]

  align(center)[
    #set text(30pt, font: "")
    计算机图形学期末项目

    #set text(20pt)
    #title

    #set text(15pt)
    #tablex(
      columns: 3,
      align: center + horizon,
      auto-vlines: false,
      header-rows: 2,

      colspanx(3)[第20小组],
      [姓名], [], [学号],
      [陈德建], [], [22336030],
      [曹越], [], [22336022],
      [王俊亚], [], [22307049],
      [张晋], [], [22336300]
    )

    #image("image/混合烟花2.png")  // 替换为你的图片路径
  
  ]
  pagebreak()

  show outline: set text(fill: rgb("#006d12"))
  outline(indent: 2.5em)
  pagebreak()

  doc
}