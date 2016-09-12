function fillTree(name, value) {
  var parts = name.split('.');

  var current = null,
    existing = null,
    i = 0;

  if (!tree.nodes||typeof tree.nodes == 'undefined')
    tree = { text: parts[y], nodes: []/*, state: { expanded: false }*/ };

  current = tree.nodes;

  for (var y = 0; y < parts.length; y++) {
    if (y != parts.length - 1) {
      existing = null;

      for (i = 0; i < current.length; i++) {
        if (current[i].text === parts[y]) {
           existing = current[i];
           break;
        }
      }

      if (existing) {
        current = existing.nodes;
      } else {
        current.push({ text: parts[y], nodes: []/*, state: { expanded: false }*/ });
        current = current[current.length - 1].nodes;
      }
    } else {
      // leaf node
      current.push({ text: parts[y], tags: [value], path: name/*, state: { disabled: true }*/ });
    }
  }

  return tree;
}

function getTree(statman) {
  // tree is an object (root) containing an array of nodes
  var tree = {};

  for (var i = 0; i < statman.length; i++)
    tree = fillTree(statman[i].name, statman[i].value);

  return tree.nodes;
}
