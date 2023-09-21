import test from 'ava'
import path from 'path'
import fs from 'fs'

import { elastixNode } from '../../dist/bundles/elastix-node.js'
import { readImageLocalFile } from 'itk-wasm'

const testDataInputDirectory = path.resolve('..', 'test', 'data', 'input')
const testDataOutputDirectory = path.resolve('..', 'test', 'data', 'output')
try {
  fs.mkdirSync(testDataOutputDirectory)
} catch (e) {
  if (e.code !== 'EEXIST') {
    throw e
  }
}


test('Default 2D registration', async t => {
  const fixedImage = await readImageLocalFile(path.join(testDataInputDirectory, 'CT_2D_head_fixed.iwi.cbor'))
  const movingImage = await readImageLocalFile(path.join(testDataInputDirectory, 'CT_2D_head_moving.iwi.cbor'))
  const transform = path.join(testDataOutputDirectory, 'CT_2D_head_node_transform.txt')
  const { result } = await elastixNode({ fixed: fixedImage, moving: movingImage, transformPath: transform })

  t.is(result.imageType.dimension, 2)
  t.is(result.imageType.componentType, 'int16')
  t.is(result.size[0], 256)
  t.is(result.size[1], 256)
})