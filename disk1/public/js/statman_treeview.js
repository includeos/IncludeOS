function fillTree(tree, name, val) {
  var parts = name.split('.');

  var current = null,
    existing = null,
    i = 0;

  if (!tree.nodes || typeof tree.nodes == 'undefined')
    tree = { text: parts[y], nodes: [] };

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
        current.push({ id: i, text: parts[y], nodes: [] });
        current = current[current.length - 1].nodes;
      }
    } else {
      // leaf node
      current.push({ id: i, text: parts[y], value: val, path: name });
    }
  }
}
