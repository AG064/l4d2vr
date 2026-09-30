import json
from pathlib import Path
import unittest


class InputProfileTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.folder = Path(__file__).parents[1] / 'L4D2VR/SteamVRActionManifest'
        cls.manifest = json.loads((cls.folder / 'action_manifest.json').read_text(encoding='utf-8-sig'))
        cls.profile = json.loads((cls.folder / 'bindings_oculus_touch_manual_reload.json').read_text(encoding='utf-8-sig'))

    def test_every_profile_output_is_declared(self):
        actions = {action['name'].lower() for action in self.manifest['actions']}
        for binding in self.profile['bindings'].values():
            for source in binding.get('sources', []):
                for value in source.get('inputs', {}).values():
                    self.assertIn(value['output'].lower(), actions)
            for kind in ('poses', 'haptics', 'skeleton'):
                for value in binding.get(kind, []):
                    self.assertIn(value['output'].lower(), actions)

    def test_optional_actions_do_not_require_rebinding_other_controllers(self):
        actions = {action['name']: action for action in self.manifest['actions']}
        for name in ('OffHandGrip', 'MagazineRelease'):
            self.assertEqual(actions['/actions/main/in/' + name]['requirement'], 'optional')

    def test_reload_controls_have_distinct_gameplay_buttons(self):
        sources = self.profile['bindings']['/actions/main']['sources']
        gameplay = ('MagazineRelease', 'OffHandGrip', 'Flashlight', 'Jump')
        paths = {}
        for source in sources:
            for value in source.get('inputs', {}).values():
                action = value['output'].split('/')[-1]
                if action in gameplay:
                    paths[action] = source['path']
        self.assertEqual(paths['MagazineRelease'], '/user/hand/right/input/a')
        self.assertEqual(paths['OffHandGrip'], '/user/hand/left/input/grip')
        self.assertEqual(paths['Flashlight'], '/user/hand/left/input/x')
        self.assertEqual(paths['Jump'], '/user/hand/right/input/joystick')
        self.assertEqual(len(set(paths.values())), len(gameplay))


if __name__ == '__main__':
    unittest.main()
