import os
import time
import unittest
import rclpy
import launch
import launch_ros.actions
import launch_testing
import launch_testing.actions
import launch_testing.asserts

def generate_test_description():
    test_db_path = "/tmp/test_integration_gateway.db"
    if os.path.exists(test_db_path):
        os.remove(test_db_path)

    gateway_node = launch_ros.actions.Node(
        package='secure_telemetry_gateway',
        executable='secure_telemetry_gateway_node',
        name='test_secure_telemetry_gateway',
        parameters=[{
            'db_path': test_db_path,
            'rate_limit_fps': 100.0,
            'enable_encryption': True
        }],
        output='screen'
    )

    return launch.LaunchDescription([
        gateway_node,
        launch_testing.actions.ReadyToTest()
    ]), {'gateway_node': gateway_node, 'test_db_path': test_db_path}


class TestGatewayIntegration(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        rclpy.init()

    @classmethod
    def tearDownClass(cls):
        rclpy.shutdown()

    def test_gateway_node_launches_cleanly(self, gateway_node, test_db_path):
        """Verify node initializes and active node graph includes gateway."""
        node = rclpy.create_node('test_verifier_client')
        
        # ROS 2 discovery takes time. Poll the node graph until the gateway appears.
        timeout = 5.0
        start_time = time.time()
        found = False
        node_names = []
        
        while time.time() - start_time < timeout:
            node_names = node.get_node_names()
            if 'test_secure_telemetry_gateway' in node_names:
                found = True
                break
            time.sleep(0.5)
            
        self.assertTrue(found, f"'test_secure_telemetry_gateway' not found. Active nodes: {node_names}")
        node.destroy_node()


@launch_testing.post_shutdown_test()
class TestGatewayShutdown(unittest.TestCase):

    def test_exit_codes(self, proc_info):
        """Verify node shuts down with exit code 0."""
        launch_testing.asserts.assertExitCodes(proc_info)
